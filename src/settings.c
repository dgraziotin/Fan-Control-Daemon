/*
 *    settings version 1.0.1
 *
 *    ANSI C implementation for managing application settings.
 *
 *    Version History:
 *    1.0.0 (2009) - Initial release
 *    1.0.1 (2010) - Fixed small memory leak in settings_delete
 *                   (Thanks to Edwin van den Oetelaar)
 *    1.0.2 (2011) - Adapted code for new strmap API
 *
 *    settings.c
 *
 *    Copyright (c) 2009-2011 Per Ola Kristensson.
 *
 *    Per Ola Kristensson <pok21@cam.ac.uk>
 *    Inference Group, Department of Physics
 *    University of Cambridge
 *    Cavendish Laboratory
 *    JJ Thomson Avenue
 *    CB3 0HE Cambridge
 *    United Kingdom
 *
 *    settings is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    settings is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with settings.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "settings.h"

#define MAX_SECTIONCHARS	256
#define MAX_KEYCHARS	256
#define MAX_VALUECHARS	256
#define MAX_LINECHARS	(MAX_KEYCHARS + MAX_VALUECHARS + 10)

#define COMMENT_CHAR	'#'
#define SECTION_START_CHAR	'['
#define SECTION_END_CHAR	']'
#define KEY_VALUE_SEPARATOR_CHAR	'='

#define DEFAULT_STRMAP_CAPACITY	256

typedef struct Section Section;
typedef struct ParseState ParseState;

struct Settings {
	Section *sections;
	unsigned int section_count;
};

struct Section {
	char *name;
	StrMap *map;
};

struct ParseState {
	char *current_section;
	unsigned int current_section_n;
	int has_section;
};

enum ConvertMode {
	CONVERT_MODE_INT, CONVERT_MODE_LONG, CONVERT_MODE_DOUBLE,
};

typedef enum ConvertMode ConvertMode;

static void trim_str(const char *str, char *out_buf);
static int parse_str(Settings *settings, char *str, ParseState *parse_state);
static int is_blank_char(char c);
static int is_blank_str(const char *str);
static int is_comment_str(const char *str);
static int is_section_str(const char *str);
static int is_key_value_str(const char *str);
static int is_key_without_value_str(const char *str);
static const char* get_token(char *str, char delim, char **last);
static int get_section_from_str(const char *str, char *out_buf,
		unsigned int out_buf_n);
static int get_key_value_from_str(const char *str, char *out_buf1,
		unsigned int out_buf1_n, char *out_buf2, unsigned int out_buf2_n);
static int get_key_without_value_from_str(const char *str, char *out_buf,
		unsigned int out_buf_n);
static int get_converted_value(const Settings *settings, const char *section,
		const char *key, ConvertMode mode, void *out);
static int get_converted_tuple(const Settings *settings, const char *section,
		const char *key, char delim, ConvertMode mode, void *out,
		unsigned int n_out);
static Section* get_section(Section *sections, unsigned int n, const char *name);
static void enum_map(const char *key, const char *value, const void *obj);

Settings* settings_new() {
	Settings *settings;

	settings = (Settings*) malloc(sizeof(Settings));

	if (settings == NULL) {
		return NULL;
	}

	settings->section_count = 0;
	settings->sections = NULL;
	return settings;
}

void settings_delete(Settings *settings) {
	unsigned int i, n;
	Section *section;

	if (settings == NULL) {
		return;
	}

	section = settings->sections;
	n = settings->section_count;
	i = 0;

	while (i < n) {
		sm_delete(section->map);

		if (section->name != NULL) {
			free(section->name);
		}

		section++;
		i++;
	}

	free(settings->sections);
	free(settings);
}

Settings* settings_open(FILE *stream) {
	Settings *settings;
	char buf[MAX_LINECHARS];
	char trimmed_buf[MAX_LINECHARS];
	char section_buf[MAX_LINECHARS];
	ParseState parse_state;

	if (stream == NULL) {
		return NULL;
	}

	settings = settings_new();

	if (settings == NULL) {
		return NULL;
	}

	parse_state.current_section = section_buf;
	parse_state.current_section_n = sizeof(section_buf);
	parse_state.has_section = 0;
	trim_str("", trimmed_buf);

	while (fgets(buf, MAX_LINECHARS, stream) != NULL) {
		trim_str(buf, trimmed_buf);

		if (!parse_str(settings, trimmed_buf, &parse_state)) {
			return NULL;
		}
	}

	return settings;
}

int settings_save(const Settings *settings, FILE *stream) {
	unsigned int i, n;
	Section *section;
	char buf[MAX_LINECHARS];

	if (settings == NULL) {
		return 0;
	}

	if (stream == NULL) {
		return 0;
	}

	section = settings->sections;
	n = settings->section_count;
	i = 0;

	while (i < n) {
		sprintf(buf, "[%s]\n", section->name);
		fputs(buf, stream);
		sm_enum(section->map, enum_map, stream);
		section++;
		i++;
		fputs("\n", stream);
	}

	return 0;
}

int settings_get(const Settings *settings, const char *section, const char *key,
		char *out_buf, unsigned int n_out_buf) {
	Section *s;

	if (settings == NULL) {
		return 0;
	}

	s = get_section(settings->sections, settings->section_count, section);

	if (s == NULL) {
		return 0;
	}

	return sm_get(s->map, key, out_buf, n_out_buf);
}

int settings_get_int(const Settings *settings, const char *section,
		const char *key) {
	int i;

	if (get_converted_value(settings, section, key, CONVERT_MODE_INT, &i)) {
		return i;
	}

	return 0;
}

long settings_get_long(const Settings *settings, const char *section,
		const char *key) {
	long l;

	if (get_converted_value(settings, section, key, CONVERT_MODE_LONG, &l)) {
		return l;
	}

	return 0;
}

double settings_get_double(const Settings *settings, const char *section,
		const char *key) {
	double d;

	if (get_converted_value(settings, section, key, CONVERT_MODE_DOUBLE, &d)) {
		return d;
	}

	return 0;
}

int settings_get_int_tuple(const Settings *settings, const char *section,
		const char *key, int *out, unsigned int n_out) {
	return get_converted_tuple(settings, section, key, ',', CONVERT_MODE_INT,
			out, n_out);
}

long settings_get_long_tuple(const Settings *settings, const char *section,
		const char *key, long *out, unsigned int n_out) {
	return get_converted_tuple(settings, section, key, ',', CONVERT_MODE_LONG,
			out, n_out);
}

double settings_get_double_tuple(const Settings *settings, const char *section,
		const char *key, double *out, unsigned int n_out) {
	return get_converted_tuple(settings, section, key, ',', CONVERT_MODE_DOUBLE,
			out, n_out);
}

int settings_set(Settings *settings, const char *section, const char *key,
		const char *value) {
	Section *s;

	if (settings == NULL) {
		return 0;
	}

	if (section == NULL || key == NULL || value == NULL) {
		return 0;
	}

	if (strlen(section) == 0) {
		return 0;
	}

	/* Get a pointer to the section */
	s = get_section(settings->sections, settings->section_count, section);

	if (s == NULL) {
		/* The section is not created---create it */
		s = (Section*) realloc(settings->sections,
				(settings->section_count + 1) * sizeof(Section));

		if (s == NULL) {
			return 0;
		}

		settings->sections = s;
		settings->section_count++;
		s = &(settings->sections[settings->section_count - 1]);
		s->map = sm_new(DEFAULT_STRMAP_CAPACITY);

		if (s->map == NULL) {
			free(s);
			return 0;
		}

		s->name = (char*) malloc((strlen(section) + 1) * sizeof(char));

		if (s->name == NULL) {
			sm_delete(s->map);
			free(s);
			return 0;
		}

		strcpy(s->name, section);
	}

	return sm_put(s->map, key, value);
}

int settings_section_get_count(const Settings *settings, const char *section) {
	Section *sect;

	if (settings == NULL) {
		return 0;
	}

	sect = get_section(settings->sections, settings->section_count, section);

	if (sect == NULL) {
		return 0;
	}

	return sm_get_count(sect->map);
}

int settings_section_enum(const Settings *settings, const char *section,
		settings_section_enum_func enum_func, const void *obj) {
	Section *sect;

	sect = get_section(settings->sections, settings->section_count, section);

	if (sect == NULL) {
		return 0;
	}

	return sm_enum(sect->map, enum_func, obj);
}

/* Copies a trimmed variant without leading and trailing blank characters
 * of the input string into the output buffer. The output buffer is assumed
 * to be large enough to contain the entire input string.
 */
static void trim_str(const char *str, char *out_buf) {
	unsigned int len;
	const char *s0;

	while (*str != '\0' && is_blank_char(*str)) {
		str++;
	}

	s0 = str;
	len = 0;

	while (*str != '\0') {
		len++;
		str++;
	}

	if (len > 0) {
		str--;
	}

	while (is_blank_char(*str)) {
		str--;
		len--;
	}

	memcpy(out_buf, s0, len);
	out_buf[len] = '\0';
}

/* Parses a single input string and updates the provided settings object.
 * The given parse state may be updated following a call. It is assumed this
 * function is called in repeated succession for each input line read. The
 * provided parse state should be initialized to the following before this
 * function is called for the first time for an intended parse:
 *
 * parse_state->current_section: a pre-allocated character buffer this function
 * can read and write to
 * parse_state->current_section_n: sizeof(parse_state->current_section)
 * parse_state->has_section: 0 (false)
 */
static int parse_str(Settings *settings, char *str, ParseState *parse_state) {
	char buf[MAX_LINECHARS];
	char buf1[MAX_LINECHARS];
	char buf2[MAX_LINECHARS];
	int result;

	if (*str == '\0') {
		return 1;

	} else if (is_blank_str(str)) {
		return 1;

	} else if (is_comment_str(str)) {
		return 1;

	} else if (is_section_str(str)) {
		result = get_section_from_str(str, buf, sizeof(buf));

		if (!result) {
			return 0;
		}

		if (strlen(buf) + 1 > parse_state->current_section_n) {
			return 0;
		}

		strcpy(parse_state->current_section, buf);
		parse_state->has_section = 1;
		return 1;

	} else if (is_key_value_str(str)) {
		result = get_key_value_from_str(str, buf1, sizeof(buf1), buf2,
				sizeof(buf2));

		if (!result) {
			return 0;
		}

		if (!parse_state->has_section) {
			return 0;
		}

		return settings_set(settings, parse_state->current_section, buf1, buf2);

	} else if (is_key_without_value_str(str)) {
		result = get_key_without_value_from_str(str, buf, sizeof(buf));

		if (!result) {
			return 0;
		}

		if (!parse_state->has_section) {
			return 0;
		}

		return settings_set(settings, parse_state->current_section, buf, "");

	} else {
		return 0;
	}
}

/* Returns true if the input character is blank,
 * false otherwise.
 */
static int is_blank_char(char c) {
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

/* Returns true if the input string is blank,
 * false otherwise.
 */
static int is_blank_str(const char *str) {
	while (*str != '\0') {
		if (!is_blank_char(*str)) {
			return 0;
		}

		str++;
	}

	return 1;
}

/* Returns true if the input string denotes a comment,
 * false otherwise.
 */
static int is_comment_str(const char *str) {
	if (*str == COMMENT_CHAR) {
		/* To be a comment the first character must be the
		 * comment character.
		 */
		return 1;
	}

	return 0;
}

/* Returns true if the input string denotes a section name,
 * false otherwise.
 */
static int is_section_str(const char *str) {
	if (*str != SECTION_START_CHAR) {
		/* The first character must be the section start character */
		return 0;
	}

	while (*str != '\0' && *str != SECTION_END_CHAR) {
		str++;
	}

	if (*str != SECTION_END_CHAR) {
		/* The section end character must be present somewhere thereafter */
		return 0;
	}

	return 1;
}

/* Returns true if the input string denotes a key-value pair,
 * false otherwise.
 */
static int is_key_value_str(const char *str) {
	if (*str == KEY_VALUE_SEPARATOR_CHAR) {
		/* It is illegal to start with the key-value separator */
		return 0;
	}

	while (*str != '\0' && *str != KEY_VALUE_SEPARATOR_CHAR) {
		str++;
	}

	if (*str != KEY_VALUE_SEPARATOR_CHAR) {
		/* The key-value separator must be present after the key part */
		return 0;
	}

	return 1;
}

/* Returns true if the input string denotes a key without a value,
 * false otherwise.
 */
static int is_key_without_value_str(const char *str) {
	if (*str == KEY_VALUE_SEPARATOR_CHAR) {
		/* It is illegal to start with the key-value separator */
		return 0;
	}

	while (*str != '\0' && *str != KEY_VALUE_SEPARATOR_CHAR) {
		str++;
	}

	if (*str == KEY_VALUE_SEPARATOR_CHAR) {
		/* The key-value separator must not be present after the key part */
		return 0;
	}

	return 1;
}

/*
 * Parses a section name from an input string. The input string is assumed to
 * already have been identified as a valid input string denoting a section name.
 */
static int get_section_from_str(const char *str, char *out_buf,
		unsigned int out_buf_n) {
	unsigned int count;

	count = 0;
	/* Jump past the section begin character */
	str++;

	while (*str != '\0' && *str != SECTION_END_CHAR) {
		/* Read in the section name into the output buffer */
		if (count == out_buf_n) {
			return 0;
		}

		*out_buf = *str;
		out_buf++;
		str++;
		count++;
	}

	/* Terminate the output buffer */
	if (count == out_buf_n) {
		return 0;
	}

	*out_buf = '\0';
	return 1;
}

/*
 * Parses a key and value from an input string. The input string is assumed to
 * already have been identified as a valid input string denoting a key-value pair.
 */
static int get_key_value_from_str(const char *str, char *out_buf1,
		unsigned int out_buf1_n, char *out_buf2, unsigned int out_buf2_n) {
	unsigned int count1;
	unsigned int count2;

	count1 = 0;
	count2 = 0;

	/* Read the key value from the input string and write it sequentially
	 * to the first output buffer by walking the input string until we either hit
	 * the null-terminator or the key-value separator.
	 */
	while (*str != '\0' && *str != KEY_VALUE_SEPARATOR_CHAR) {
		/* Ensure the first output buffer is large enough. */
		if (count1 == out_buf1_n) {
			return 0;
		}

		/* Copy the character to the first output buffer */
		*out_buf1 = *str;
		out_buf1++;
		str++;
		count1++;
	}

	/* Terminate the first output buffer */
	if (count1 == out_buf1_n) {
		return 0;
	}

	*out_buf1 = '\0';

	/* Now trace the first input buffer backwards until we hit a non-blank character */
	while (is_blank_char(*(out_buf1 - 1))) {
		out_buf1--;
	}

	*out_buf1 = '\0';

	/* Try to proceed one more character, past the last read key-value
	 * delimiter, in the input string.
	 */
	if (*str != '\0') {
		str++;
	}

	/* Now find start of the value in the input string by walking the input
	 * string until we either hit the null-terminator or a blank character.
	 */
	while (*str != '\0' && is_blank_char(*str)) {
		str++;
	}

	while (*str != '\0') {
		/* Fail if there is a possibility that we are overwriting the second
		 * input buffer.
		 */
		if (count2 == out_buf2_n) {
			return 0;
		}

		/* Copy the character to the second output buffer */
		*out_buf2 = *str;
		out_buf2++;
		str++;
		count2++;
	}

	/* Terminate the second output buffer */
	if (count2 == out_buf2_n) {
		return 0;
	}

	*out_buf2 = '\0';
	return 1;
}

/*
 * Parses a key from an input string. The input string is assumed to already
 * have been identified as a valid input string denoting a key without a value.
 */
static int get_key_without_value_from_str(const char *str, char *out_buf,
		unsigned int out_buf_n) {
	unsigned int count;

	count = 0;

	/* Now read the key value from the input string and write it sequentially
	 * to the output buffer by walking the input string until we either hit
	 * the null-terminator or the key-value separator.
	 */
	while (*str != '\0') {
		/* Ensure the output buffer is large enough. */
		if (count == out_buf_n) {
			return 0;
		}

		/* Copy the character to the input buffer */
		*out_buf = *str;
		out_buf++;
		str++;
		count++;
	}

	/* Terminate the output buffer */
	if (count == out_buf_n) {
		return 0;
	}

	*out_buf = '\0';
	return 1;
}

/* Returns a pointer to the next token in the input string delimited
 * by the specified delimiter or null if no such token exist. The provided
 * last pointer will be changed to point one position after the pointed
 * token. The currently ouputted token will be null-terminated.
 *
 * An idiom for tokenizing a (in this case, comma-separated) string is:
 *
 * char test_string[] = "Token1,Token2,Token3";
 * char token[255];
 * char *str;
 *
 * str = test_string;
 * while ((token = get_token(str, ',', &str) != NULL) {
 *     printf("token: %s", token);
 * }
 */
static const char* get_token(char *str, char delim, char **last) {

	char *s0;

	s0 = str;

	/* If we hit the null-terminator the string
	 * is exhausted and another token does not
	 * exist.
	 */
	if (*str == '\0') {
		return NULL;
	}

	/* Walk the string until we encounter a
	 * null-terminator or the delimiter.
	 */
	while (*str != '\0' && *str != delim) {
		str++;
	}

	/* Terminate the return token, if necessary */
	if (*str != '\0') {
		*str = '\0';
		str++;
	}

	*last = str;
	return s0;
}

/* Returns a converted value pointed to by the provided key in the given section.
 * The mode specifies which conversion takes place and dictates what value out
 * is pointing to. The value out is pointing to will be replaced by the converted
 * value assuming conversion is succesful. The function returns 1 if conversion
 * is succsessful and 0 if the convertion could not be carried out.
 */
static int get_converted_value(const Settings *settings, const char *section,
		const char *key, ConvertMode mode, void *out) {
	char value[MAX_VALUECHARS];

	if (!settings_get(settings, section, key, value, MAX_VALUECHARS)) {
		return 0;
	}

	switch (mode) {
	case CONVERT_MODE_INT:
		*((int*) out) = atoi(value);
		return 1;

	case CONVERT_MODE_LONG:
		*((long*) out) = atol(value);
		return 1;

	case CONVERT_MODE_DOUBLE:
		*((double*) out) = atof(value);
		return 1;
	}

	return 0;
}

/* Returns a converted tuple pointed to by the provided key in the given section.
 * The tuple is created by splitting the value by the supplied delimiter and then
 * converting each token after the split according to the specified mode.
 * The array out is pointing to will be replaced by the converted tuple
 * assuming conversion is succesful. The function returns 1 if conversion
 * is succsessful and 0 if the convertion could not be carried out.
 */
static int get_converted_tuple(const Settings *settings, const char *section,
		const char *key, char delim, ConvertMode mode, void *out,
		unsigned int n_out) {
	unsigned int count;
	const char *token;
	static char value[MAX_VALUECHARS];
	char *v;

	if (out == NULL) {
		return 0;
	}

	if (n_out == 0) {
		return 0;
	}

	if (!settings_get(settings, section, key, value, MAX_VALUECHARS)) {
		return 0;
	}

	v = value;
	count = 0;

	/* Walk over all tokens in the value, and convert them and assign them
	 * to the output array as specified by the mode.
	 */
	while ((token = get_token(v, delim, &v)) != NULL && count < n_out) {
		switch (mode) {
		case CONVERT_MODE_INT:
			((int*) out)[count] = atoi(token);
			break;

		case CONVERT_MODE_LONG:
			((long*) out)[count] = atol(token);
			break;

		case CONVERT_MODE_DOUBLE:
			((double*) out)[count] = atof(token);
			break;

		default:
			return 0;
		}

		count++;
	}

	return 1;
}

/* Returns a pointer to the section or null if the named section does not
 * exist.
 */
static Section* get_section(Section *sections, unsigned int n, const char *name) {
	unsigned int i;
	Section *section;

	if (name == NULL) {
		return NULL;
	}

	section = sections;
	i = 0;

	while (i < n) {
		if (strcmp(section->name, name) == 0) {
			return section;
		}

		section++;
		i++;
	}

	return NULL;
}

/* Callback function that is passed into the enumeration function in the
 * string map. It casts the passed into object into a FILE pointer and
 * writes out the key and value to the file.
 */
static void enum_map(const char *key, const char *value, const void *obj) {
	FILE *stream;
	char buf[MAX_LINECHARS];

	if (key == NULL || value == NULL) {
		return;
	}

	if (obj == NULL) {
		return;
	}

	stream = (FILE*) obj;

	if (strlen(key) < MAX_KEYCHARS && strlen(value) < MAX_VALUECHARS) {
		sprintf(buf, "%s%c%s\n", key, KEY_VALUE_SEPARATOR_CHAR, value);
		fputs(buf, stream);
	}
}

/*

 GNU LESSER GENERAL PUBLIC LICENSE
 Version 3, 29 June 2007

 Copyright (C) 2007 Free Software Foundation, Inc. <http://fsf.org/>
 Everyone is permitted to copy and distribute verbatim copies
 of this license document, but changing it is not allowed.


 This version of the GNU Lesser General Public License incorporates
 the terms and conditions of version 3 of the GNU General Public
 License, supplemented by the additional permissions listed below.

 0. Additional Definitions.

 As used herein, "this License" refers to version 3 of the GNU Lesser
 General Public License, and the "GNU GPL" refers to version 3 of the GNU
 General Public License.

 "The Library" refers to a covered work governed by this License,
 other than an Application or a Combined Work as defined below.

 An "Application" is any work that makes use of an interface provided
 by the Library, but which is not otherwise based on the Library.
 Defining a subclass of a class defined by the Library is deemed a mode
 of using an interface provided by the Library.

 A "Combined Work" is a work produced by combining or linking an
 Application with the Library.  The particular version of the Library
 with which the Combined Work was made is also called the "Linked
 Version".

 The "Minimal Corresponding Source" for a Combined Work means the
 Corresponding Source for the Combined Work, excluding any source code
 for portions of the Combined Work that, considered in isolation, are
 based on the Application, and not on the Linked Version.

 The "Corresponding Application Code" for a Combined Work means the
 object code and/or source code for the Application, including any data
 and utility programs needed for reproducing the Combined Work from the
 Application, but excluding the System Libraries of the Combined Work.

 1. Exception to Section 3 of the GNU GPL.

 You may convey a covered work under sections 3 and 4 of this License
 without being bound by section 3 of the GNU GPL.

 2. Conveying Modified Versions.

 If you modify a copy of the Library, and, in your modifications, a
 facility refers to a function or data to be supplied by an Application
 that uses the facility (other than as an argument passed when the
 facility is invoked), then you may convey a copy of the modified
 version:

 a) under this License, provided that you make a good faith effort to
 ensure that, in the event an Application does not supply the
 function or data, the facility still operates, and performs
 whatever part of its purpose remains meaningful, or

 b) under the GNU GPL, with none of the additional permissions of
 this License applicable to that copy.

 3. Object Code Incorporating Material from Library Header Files.

 The object code form of an Application may incorporate material from
 a header file that is part of the Library.  You may convey such object
 code under terms of your choice, provided that, if the incorporated
 material is not limited to numerical parameters, data structure
 layouts and accessors, or small macros, inline functions and templates
 (ten or fewer lines in length), you do both of the following:

 a) Give prominent notice with each copy of the object code that the
 Library is used in it and that the Library and its use are
 covered by this License.

 b) Accompany the object code with a copy of the GNU GPL and this license
 document.

 4. Combined Works.

 You may convey a Combined Work under terms of your choice that,
 taken together, effectively do not restrict modification of the
 portions of the Library contained in the Combined Work and reverse
 engineering for debugging such modifications, if you also do each of
 the following:

 a) Give prominent notice with each copy of the Combined Work that
 the Library is used in it and that the Library and its use are
 covered by this License.

 b) Accompany the Combined Work with a copy of the GNU GPL and this license
 document.

 c) For a Combined Work that displays copyright notices during
 execution, include the copyright notice for the Library among
 these notices, as well as a reference directing the user to the
 copies of the GNU GPL and this license document.

 d) Do one of the following:

 0) Convey the Minimal Corresponding Source under the terms of this
 License, and the Corresponding Application Code in a form
 suitable for, and under terms that permit, the user to
 recombine or relink the Application with a modified version of
 the Linked Version to produce a modified Combined Work, in the
 manner specified by section 6 of the GNU GPL for conveying
 Corresponding Source.

 1) Use a suitable shared library mechanism for linking with the
 Library.  A suitable mechanism is one that (a) uses at run time
 a copy of the Library already present on the user's computer
 system, and (b) will operate properly with a modified version
 of the Library that is interface-compatible with the Linked
 Version.

 e) Provide Installation Information, but only if you would otherwise
 be required to provide such information under section 6 of the
 GNU GPL, and only to the extent that such information is
 necessary to install and execute a modified version of the
 Combined Work produced by recombining or relinking the
 Application with a modified version of the Linked Version. (If
 you use option 4d0, the Installation Information must accompany
 the Minimal Corresponding Source and Corresponding Application
 Code. If you use option 4d1, you must provide the Installation
 Information in the manner specified by section 6 of the GNU GPL
 for conveying Corresponding Source.)

 5. Combined Libraries.

 You may place library facilities that are a work based on the
 Library side by side in a single library together with other library
 facilities that are not Applications and are not covered by this
 License, and convey such a combined library under terms of your
 choice, if you do both of the following:

 a) Accompany the combined library with a copy of the same work based
 on the Library, uncombined with any other library facilities,
 conveyed under the terms of this License.

 b) Give prominent notice with the combined library that part of it
 is a work based on the Library, and explaining where to find the
 accompanying uncombined form of the same work.

 6. Revised Versions of the GNU Lesser General Public License.

 The Free Software Foundation may publish revised and/or new versions
 of the GNU Lesser General Public License from time to time. Such new
 versions will be similar in spirit to the present version, but may
 differ in detail to address new problems or concerns.

 Each version is given a distinguishing version number. If the
 Library as you received it specifies that a certain numbered version
 of the GNU Lesser General Public License "or any later version"
 applies to it, you have the option of following the terms and
 conditions either of that published version or of any later version
 published by the Free Software Foundation. If the Library as you
 received it does not specify a version number of the GNU Lesser
 General Public License, you may choose any version of the GNU Lesser
 General Public License ever published by the Free Software Foundation.

 If the Library as you received it specifies that a proxy can decide
 whether future versions of the GNU Lesser General Public License shall
 apply, that proxy's public statement of acceptance of any version is
 permanent authorization for you to choose that version for the
 Library.

 */
