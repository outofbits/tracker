/*
 * Copyright (C) 2010, Nokia <ivan.frade@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include <string.h>

#include <glib.h>
#include <gio/gio.h>

#include <libtracker-fts/tracker-parser.h>

/* -------------- COMMON FOR ALL TESTS ----------------- */

/* Fixture object type */
typedef struct {
	/* The parser object */
	TrackerParser    *parser;

	/* Default parser configuration to use */
	gint              max_word_length;
	gboolean          delimit_words;
	gboolean          enable_stemmer;
	gboolean          enable_stop_words;
	gboolean          skip_reserved_words;
	gboolean          skip_numbers;
} TrackerParserTestFixture;

/* Common setup for all tests */
static void
test_common_setup (TrackerParserTestFixture *fixture,
                   gconstpointer data)
{
	TrackerLanguage  *language;

	/* Setup language for parser */
	language = tracker_language_new (NULL);
	if (!language) {
		g_critical ("Language setup failed!");
		return;
	}

	/* Default conf parameters */
	fixture->max_word_length = 50;
	fixture->delimit_words = TRUE;
	fixture->enable_stemmer = TRUE;
	fixture->enable_stop_words = TRUE;
	fixture->skip_reserved_words = TRUE;
	fixture->skip_numbers = TRUE;

	/* Create the parser */
	fixture->parser = tracker_parser_new (language,
	                                      fixture->max_word_length);
	if (!fixture->parser) {
		g_critical ("Parser creation failed!");
		return;
	}

	g_object_unref (language);
}

/* Common teardown for all tests */
static void
test_common_teardown (TrackerParserTestFixture *fixture,
                      gconstpointer data)
{
	if (fixture->parser) {
		tracker_parser_free (fixture->parser);
	}
}

/* -------------- EXPECTED NUMBER OF WORDS TESTS ----------------- */

/* Test struct for the expected-nwords tests */
typedef struct TestDataExpectedNWords TestDataExpectedNWords;
struct TestDataExpectedNWords {
	const gchar *str;
	gboolean     skip_numbers;
	guint        expected_nwords;
};

/* Common expected_word test method */
static void
expected_nwords_check (TrackerParserTestFixture *fixture,
                       gconstpointer data)
{
	const TestDataExpectedNWords *testdata = data;
	const gchar *word;
	gint position;
	gint byte_offset_start;
	gint byte_offset_end;
	gboolean stop_word;
	gint word_length;
	guint nwords = 0;

	/* Reset the parser with the test string */
	tracker_parser_reset (fixture->parser,
	                      testdata->str,
	                      strlen (testdata->str),
	                      fixture->delimit_words,
	                      fixture->enable_stemmer,
	                      fixture->enable_stop_words,
	                      fixture->skip_reserved_words,
	                      testdata->skip_numbers);

	/* Count number of output words */
	while ((word = tracker_parser_next (fixture->parser,
	                                    &position,
	                                    &byte_offset_start,
	                                    &byte_offset_end,
	                                    &stop_word,
	                                    &word_length))) {
		nwords++;
	}

	/* Check if input is same as expected */
	g_assert_cmpuint (nwords, == , testdata->expected_nwords);
}

/* -------------- EXPECTED WORD TESTS ----------------- */

/* Test struct for the expected-word tests */
typedef struct TestDataExpectedWord TestDataExpectedWord;
struct TestDataExpectedWord {
	const gchar  *str;
	const gchar  *expected;
};

/* Common expected_word test method */
static void
expected_word_check (TrackerParserTestFixture *fixture,
                     gconstpointer data)
{
	const TestDataExpectedWord *testdata = data;
	const gchar *word;
	gint position;
	gint byte_offset_start;
	gint byte_offset_end;
	gboolean stop_word;
	gint word_length;

	/* Reset the parser with our string */
	tracker_parser_reset (fixture->parser,
	                      testdata->str,
	                      strlen (testdata->str),
	                      fixture->delimit_words,
	                      FALSE, /* no stemming for this test */
	                      fixture->enable_stop_words,
	                      fixture->skip_reserved_words,
	                      fixture->skip_numbers);

	/* Process next word */
	word = tracker_parser_next (fixture->parser,
	                            &position,
	                            &byte_offset_start,
	                            &byte_offset_end,
	                            &stop_word,
	                            &word_length);

	/* Check if input is same as expected */
	g_assert_cmpstr (word, == , testdata->expected);
}

/* -------------- LIST OF TESTS ----------------- */

#ifdef HAVE_UNAC
/* Normalization-related tests (unaccenting) */
static const TestDataExpectedWord test_data_normalization[] = {
	{ "école",                "ecole" },
	{ "ÉCOLE",                "ecole" },
	{ "École",                "ecole" },
	{ "e" "\xCC\x81" "cole",  "ecole" },
	{ "E" "\xCC\x81" "COLE",  "ecole" },
	{ "E" "\xCC\x81" "cole",  "ecole" },
	{ NULL,                   NULL    }
};

/* Unaccenting-related tests */
static const TestDataExpectedWord test_data_unaccent[] = {
	{ "Murciélago", "murcielago" },
	{ "camión",     "camion"     },
	{ "desagüe",    "desague"    },
	{ NULL,         NULL         }
};
#else
/* Normalization-related tests (not unaccenting) */
static const TestDataExpectedWord test_data_normalization[] = {
	{ "école",                "école" },
	{ "ÉCOLE",                "école" },
	{ "École",                "école" },
	{ "e" "\xCC\x81" "cole",  "école" },
	{ "E" "\xCC\x81" "COLE",  "école" },
	{ "E" "\xCC\x81" "cole",  "école" },
	{ NULL,                   NULL    }
};
#endif

/* Casefolding-related tests */
static const TestDataExpectedWord test_data_casefolding[] = {
	{ "gross", "gross" },
	{ "GROSS", "gross" },
	{ "GrOsS", "gross" },
	{ "groß",  "gross" },
	{ NULL,    NULL    }
};

/* Number of expected words tests */
static const TestDataExpectedNWords test_data_nwords[] = {
	{ "The quick (\"brown\") fox can’t jump 32.3 feet, right?", TRUE,   8 },
	{ "The quick (\"brown\") fox can’t jump 32.3 feet, right?", FALSE,  9 },
	{ "ホモ・サピエンス",                                            TRUE,   2 }, /* katakana */
	{ "本州最主流的风味",                                          TRUE,   8 }, /* chinese */
	{ "Американские суда находятся в международных водах.",     TRUE,   6 }, /* russian */
	{ "Bần chỉ là một anh nghèo xác",                           TRUE,   7 }, /* vietnamese */
	{ "ホモ・サピエンス 本州最主流的风味 katakana, chinese, english",   TRUE,  13 }, /* mixed */
	{ NULL,                                                     FALSE,  0 }
};

int
main (int argc, char **argv)
{
	gint i;

	g_type_init ();
	if (!g_thread_supported ()) {
		g_thread_init (NULL);
	}
	g_test_init (&argc, &argv, NULL);

	/* Add normalization checks */
	for (i = 0; test_data_normalization[i].str != NULL; i++) {
		gchar *testpath;

		testpath = g_strdup_printf ("/libtracker-fts/parser/normalization_%d", i);
		g_test_add (testpath,
		            TrackerParserTestFixture,
		            &test_data_normalization[i],
		            test_common_setup,
		            expected_word_check,
		            test_common_teardown);
		g_free (testpath);
	}

#ifdef HAVE_UNAC
	/* Add unaccent checks */
	for (i = 0; test_data_unaccent[i].str != NULL; i++) {
		gchar *testpath;

		testpath = g_strdup_printf ("/libtracker-fts/parser/unaccent_%d", i);
		g_test_add (testpath,
		            TrackerParserTestFixture,
		            &test_data_unaccent[i],
		            test_common_setup,
		            expected_word_check,
		            test_common_teardown);
		g_free (testpath);
	}
#endif

	/* Add casefolding checks */
	for (i = 0; test_data_casefolding[i].str != NULL; i++) {
		gchar *testpath;

		testpath = g_strdup_printf ("/libtracker-fts/parser/casefolding_%d", i);
		g_test_add (testpath,
		            TrackerParserTestFixture,
		            &test_data_casefolding[i],
		            test_common_setup,
		            expected_word_check,
		            test_common_teardown);
		g_free (testpath);
	}

	/* Add expected number of words checks */
	for (i = 0; test_data_nwords[i].str != NULL; i++) {
		gchar *testpath;

		testpath = g_strdup_printf ("/libtracker-fts/parser/nwords_%d", i);
		g_test_add (testpath,
		            TrackerParserTestFixture,
		            &test_data_nwords[i],
		            test_common_setup,
		            expected_nwords_check,
		            test_common_teardown);
		g_free (testpath);
	}

	return g_test_run ();
}
