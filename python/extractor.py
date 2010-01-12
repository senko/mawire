#!/usr/bin/env python
#
# Copyright (C) 2010 Senko Rasic <senko.rasic@collabora.co.uk>
#
# Wikipedia XML dump file parser
#
# Usage: python extractor.py <wikipedia_xml_file.xml> <sqlite_dbfile.db>
#
# Parse the Wikipedia XML dump file (articles) and create an
# SQLite3 database containing "articles" table with three columns,
# article title ("title"), article text ("text"), and unique autoincrementing
# integer ID. Text column is actually compressed (using zlib.compress).
# Both title column and text column when uncompressed are in utf-8 encoding.
# Integer ID is just used for quickly selecting one article at random.
#
# Most distros don't build SQLite3 with FTS3 free text search support,
# without which article search takes forever. After the database is
# populated using this program, it should be post-processed using
# sqlite3 with FTS3 enabled. Post-processing involves creating
# a new virtual table for free text search and populating it with
# article titles:
#
#    CREATE VIRTUAL TABLE article_index USING fts3();
#    INSERT INTO article_index SELECT title FROM articles;

from lxml import etree
from xml.parsers import expat
import re
import gc

import shelve
import sys
import zlib

from sqlalchemy import create_engine, func, select, and_
from sqlalchemy import Table, Column, Integer, String, Binary, DateTime, MetaData
from sqlalchemy.exc import NoSuchTableError
from sqlalchemy.sql import text

class XMLStreamExtractor(object):

    def __init__(self):
        self.parser = expat.ParserCreate()
        self.parser.StartElementHandler = self._start_handler
        self.parser.EndElementHandler = self._end_handler
        self.parser.CharacterDataHandler = self._cdata_handler
        self._current_cdata = []
        self.handle_element = lambda a, b: False
        self.filter_elements = []

    def _start_handler(self, tag, attrs):
        self._current_cdata = []

    def _cdata_handler(self, data):
        self._current_cdata.append(data)

    def _end_handler(self, tag):
        if tag in self.filter_elements:
            txt = ''.join(self._current_cdata)
            self.handle_element(tag, txt)
        self._current_cdata = []

    def run(self, fd):
        self.parser.ParseFile(fd)


class WikimediaPageParser(object):

    ## TODO: check overly-zealous parsing in "Croatian Wine" article
    ## (at the end). Change " *" into bullet point, and ":", "::", ":::", ...
    ## into lists
    def __init__(self, xml_extractor):
        self.xml_tag_pattern = re.compile(r'(<!--.*?-->|<ref.*?</ref>|<.*?>)',
            flags=re.S)
        # Ugly but neccessary because Python's regexps are not recursive :'(
        # We just hope there won't be more than 19 nested braces
        self.braces_pattern = re.compile(r'{([^{}]|{([^{}]|{([^{}]|{([^{}]|{([^{}]|{([^{}]|{([^{}]|{([^{}]|{([^{}]|{([^{}]|{([^{}]|{([^{}]|{([^{}]|{([^{}]|{([^{}]|{([^{}]|{([^{}]|{([^{}]|{([^{}]|{})*})*})*})*})*})*})*})*})*})*})*})*})*})*})*})*})*})*})*}')

        self.wikiref_pattern = re.compile(r'\[\[([^][|:]+\|)?([^][:]+)\]\]')
        self.wikiref2_pattern = re.compile(r'\[\[[A-Za-z]+:.*?\]\]')
        self.extraref_pattern = re.compile(r'\[(https?|ftp)://[^] ]+ ([^]]+)]')
        self.extraref2_pattern = re.compile(r'\[(https?|ftp)://[^]]+]')
        self.paren_fixup_pattern = re.compile(r'\([;,. -]+')

        self.space_fixup_pattern = re.compile(r'(\(\)|&nbsp;|\t| )+')
        self.entity_fixup_pattern = re.compile(r'&(mdash|ndash|quot|amp|lt|gt);')

        xml_extractor.handle_element = self._handle_element
        xml_extractor.filter_elements = [ 'title', 'text', 'page' ]

        self._current_title = None
        self._current_text = None
        self.handle_page = lambda a, b: None
        self.filter_page_raw = lambda a, b: True
        self.n_pages = 0

    def _handle_element(self, name, text):
        if name == 'title':
            self._current_title = text
        elif name == 'text':
            self._current_text = text
        elif name == 'page':
            self._handle_page(self._current_title, self._current_text)
            self._current_title = None
            self._current_text = None

    def _handle_page(self, title, text):
        self.n_pages += 1

        if not self.filter_page_raw(title, text):
            return

        if '==' in text:
            text = text.split('==')[0]

        text = self.braces_pattern.sub('', text)
        text = self.wikiref_pattern.sub(lambda x: x.groups(0)[1], text)
        text = self.wikiref2_pattern.sub('', text)
        text = self.extraref_pattern.sub(lambda x: x.groups(0)[1], text)
        text = self.extraref2_pattern.sub('', text)

        text = self.xml_tag_pattern.sub('', text)

        text = self.paren_fixup_pattern.sub('(', text)
        text = self.space_fixup_pattern.sub(' ', text)

        text = self.entity_fixup_pattern.sub(lambda x:
            { 'mdash': '-', 'ndash': '-', 'quot': '"',
                'amp': '&', 'lt': '<', 'gt': '>' }[x.groups()[0]], text)

        text = text.strip()
        self.handle_page(title, text)


class WikipediaPageFilter(object):

    def __init__(self, parser):
        parser.handle_page = self._handle_page
        parser.filter_page_raw = self._filter_page_raw
        self.minimal_text_length = 200
        self.callback = lambda a, b: None

    def _filter_page_raw(self, title, text):
        if (':' in title or
            title.startswith('List of ') or
            text.startswith('#REDIRECT ')):
                return False
        return True

    def _handle_page(self, title, text):
        if len(text) < self.minimal_text_length:
            return

        self.callback(title, text)


class ArticleStorage(object):

    # After-the-fact FTS3 enable:
    # CREATE VIRTUAL TABLE article_index USING fts3();
    # INSERT INTO article_index SELECT title FROM articles;
    #
    def __init__(self, uri):
        self.engine = create_engine(uri)
        self.md = MetaData(self.engine)

        self.conn = self.engine.connect()
        try:
            self.table = Table('articles', self.md, autoload=True)
        except NoSuchTableError:
            self.table = Table('articles', self.md,
                Column('id', Integer, primary_key=True),
                Column('title', String, unique=True, index=True),
                Column('text', Binary))
            self.md.create_all()

        # We go as fast as possible. If the system crashes in the middle,
        # the user can always rebuild the db.
        self.conn.execute(text('PRAGMA journal_mode = MEMORY;'))
        self.conn.execute(text('PRAGMA synchronous = OFF;'))

        self.trans = self.conn.begin()
        self.orig_size = 0
        self.store_size = 0
        self.n_articles = 0
        self.parser = None

    def close(self):
        self.trans.commit()
        self.conn.close()

    def store(self, title, text):
        blob = zlib.compress(text.encode('utf-8'), 9)

        self.orig_size += len(text)
        self.store_size += len(blob)
        self.n_articles += 1

        self.table.insert().execute(title=title, text=blob)

        if (self.n_articles % 100) == 0:
            sys.stderr.write("Storing article %d/%d (ratio %d%%)\n" % (self.n_articles,
                self.parser.n_pages, 100 * self.store_size / self.orig_size))
            self.trans.commit()
            self.trans = self.conn.begin()


def run(infile, outfile):
    if infile == '-':
        inp = sys.stdin
    else:
        inp = open(infile, 'r')

    x = XMLStreamExtractor()
    p = WikimediaPageParser(x)
    f = WikipediaPageFilter(p)
    s = ArticleStorage('sqlite:///%s' % outfile)
    f.callback = s.store
    s.parser = p
    x.run(inp)
    s.close()

if len(sys.argv) != 3:
    print "Usage: extractor <wikipedia_dump.xml|-> <sqlite_database.db>"
    sys.exit(-1)

run(sys.argv[1], sys.argv[2])

