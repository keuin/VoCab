/* VoCab -- A C program that helps you remember your vocabulary.
** Written by Keuin.
** Current Version: 1.0
** [2018/12/22 11:49]  First Version 1.0 @ [HIT Lib 5F]
*/

#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <stdbool.h>
#include <string.h>
#include <Windows.h>
#include <float.h>


#define VOCAB_MAX_LENGTH  48
#define TRANS_MAX_LENGTH  48
#define FILEHEAD          667755443399520UL

// Debug print
#define DBGP(text) puts(text)
#ifndef NDEBUG
#undef DBGP
#define DBGP(text) (text)
#endif

typedef unsigned short cnt_t;

typedef struct {
	unsigned long long FileHead;
	int cnt;
}file_head;

typedef struct {
	char vocab[VOCAB_MAX_LENGTH + 1];
	char trans[TRANS_MAX_LENGTH + 1];
	cnt_t test_cnt;
	cnt_t pass_cnt;
}record;

_Bool doExercise ();
void play (const file_head* head);
double compare (const record *a, const record *b);
void generateANewDatabase ();
int Query (const char* hint, const char* validOptions, int ignoreCase);

record *record_buf;
record **record_list;

#define STACK_H

#define BUF_INCREMENT  100

#define VOCAB_MAX_LENGTH  48
#define TRANS_MAX_LENGTH  48

typedef record DataType;

typedef struct {
	DataType *heap;  /* pointer to buffer                 */
	int cnt;         /* how many numbers already in stack */
	int buf_size;    /* buffer size                       */
}Stack;

Stack* create ();                     /* create an empty stack                 */
void release (Stack * st);            /* free a stack                          */
int isEmpty (Stack *st);              /* whether the stack is empty            */
int push (Stack *st, DataType data);  /* push a new number into stack          */
int _incBuf (Stack *st);              /* (internal) increase buffer of a stack */
int pop (Stack *st, DataType *retval);             /* pop a number from the top             */


int main()
{
	while (1) {
		int opt1 = Query ("What are you going to do now?\n"
			"[1] Open a database and exercise.\n"
			"[2] Generate a new database from a vocabulary list.\n"
			"[3] Quit.\n"
			, "123", 0);
		switch (opt1) {
		case '1':
			doExercise ();
			break;
		case '2':
			generateANewDatabase ();
			break;
		case '3':
			goto EXIT;
			break;
		}
	}
EXIT:
    return 0;
}

_Bool doExercise () {
	char filename[FILENAME_MAX + 1];
	_Bool retflag = true;
	FILE* f;
	file_head head;
	size_t real_cnt; /* real count of read voc */
	

	printf ("VoCab Filename(*.voc):");
	while(1) {
		gets_s (filename, FILENAME_MAX);
		if (_access (filename, 0)) {
			printf ("! File \"%s\" does not exist!\nTry again:", filename);
			continue;
		}
		break;
	}
	f = fopen (filename, "r+b");

	// Get file head
	if (fread (&head, sizeof (file_head), 1, f) != 1) {
		printf ("! Failed to access file head. Corrupted file?");
		retflag = false;
		goto RET;
	}
	if (head.FileHead != FILEHEAD) {
		printf ("! Invalid file head! Are you kidding me?\n");
		retflag = false;
		goto RET;
	}

	// Get vocabularies
	if ((record_buf = malloc (sizeof (record) * head.cnt)) == NULL) {
		printf ("! Failed to allocate a buffer! Memory full?\n");
		retflag = false;
		goto RET;
	}
	if ((real_cnt = fread (record_buf, sizeof (record), head.cnt, f)) < (unsigned)head.cnt) {
		printf ("* Record count is less than expected (%d < %d). Truncating file head.\n", real_cnt, head.cnt);
		head.cnt = real_cnt;
	}
	DBGP ("Initializing raw list.");
	if ((record_list = calloc (head.cnt, sizeof (record*))) == NULL) {
		printf ("Failed to allocate the buffer of list! Memory full?\n");
		retflag = false;
		goto RET;
	}
	for (int i = 0; i < head.cnt; ++i)
		record_list[i] = record_buf + i;
	
	do {
		play (&head);

		// Write to disk
		fseek (f, sizeof (file_head), SEEK_SET);
		fwrite (record_buf, sizeof (record), head.cnt, f);

		int opt = Query ("Congratulations! You have reviewed all of your vocabularies!\n Want to play again?", "yn", 1);
		if (opt == 'n')
			break;
	} while (1);


RET:
	fclose (f);
	if ((void *)record_buf != NULL)
		free (record_buf);
	if ((void*)record_list != NULL)
		free (record_list);
	return retflag;
}

void play (const file_head* head) {
	DBGP ("Sorting list.");
	int cc = 0;
	record *swp_tmp;
	do {
		cc = 0; // Change Count
		for (int i = 0; i < head->cnt - 1; ++i) {
			if (compare (record_list[i],record_list[i+1]) < 0) {
				swp_tmp = record_list[i + 1];
				record_list[i + 1] = record_list[i];
				record_list[i] = swp_tmp;
				++cc;
			}
		}
	} while (cc);

	DBGP ("Done! Go to play.");
	int mode = Query ("Select a mode:\n"\
		"[E] Show English, recalling Chinese.\n"
		"[C] Show Chinese, recalling English.\n", "EC", 1);

	for (record* rec = record_list[0]; rec <= record_list[head->cnt - 1]; ++rec) {
		system ("cls");
		if (mode == 'E') {
			printf ("[%-24s] <-> [%-24s]\n", rec->vocab, "?");
		} else {
			printf ("[%-24s] <-> [%-24s]\n", "?", rec->trans);
		}
		system ("pause");
		system ("cls");
		printf ("[%-24s] <-> [%-24s]\n", rec->vocab, rec->trans);
		int opt = Query ("    Can you recall it?", "YN", 1);
		++(rec->test_cnt);
		if (opt == 'Y')
			++(rec->pass_cnt);
		printf ("Success rate: %f%%  (%d of %d)\n", 100.0 * rec->pass_cnt / rec->test_cnt, rec->pass_cnt, rec->test_cnt);
		system ("pause");
	}
}

double compare (const record *a, const record *b) {
	/* value range: -1.0 ~ 1.0. Bigger when [a] is more important */
	if (a->test_cnt == 0 && b->test_cnt == 0)
		return DBL_EPSILON;
	if (b->test_cnt == 0)
		return -1;
	if (a->test_cnt == 0)
		return 1;
	return (1.0 * b->pass_cnt / b->test_cnt - 1.0 * a->pass_cnt / a->test_cnt);
}

void generateANewDatabase () {
	record rec;
	char filename[FILENAME_MAX + 1];
	file_head head = { FILEHEAD };
	Stack* stack = create ();
	FILE* f;
	if (stack == (Stack *)NULL) {
		printf ("! Failed to create a stack! Memory full?");
		return;
	}
	while (1) {
		printf ("Input dataset filename:");
		gets_s (filename, FILENAME_MAX);
		if (!_access (filename, 0)) {
			printf ("* File \"%s\" already exists, and continue to import WILL OVERWRITE THIS FILE! Really continue?", filename);
			int opt = Query("", "YN", 1);
			if (opt == 'N')
				continue;// Reconsider
		}
		break;
	}
	rec.pass_cnt = rec.test_cnt = 0;
	printf ("Input dataset. example: desert,沙漠，抛弃，遗弃. End with a blank line :\n");
	char inpbuf[256];
	int len;
	while (gets_s (inpbuf, 255)[0] != '\0') {
		len = strlen (inpbuf);
		for (int k = 0; k < VOCAB_MAX_LENGTH - 1; ++k) {
			if (inpbuf[k] == ',') {
				memcpy (rec.vocab, inpbuf, k);
				rec.vocab[k] = '\0';
				strcpy (rec.trans, inpbuf + k + 1);
				push (stack, rec);
				break;
			}
		}
	}

	printf ("%d valid record(s). Writing to file.\n", stack->cnt);
	f = fopen (filename, "wb");
	head.cnt = stack->cnt;
	fwrite (&head, sizeof (file_head), 1, f);
	size_t sz = fwrite (stack->heap, sizeof (record), head.cnt, f); // The stack.h is written by myself and I hacked it ;)
	fclose (f);

	release (stack);
	printf ("Finished. Wrote %u record(s).\n", (unsigned)sz);
}

char lower (char i) {
	/* a simple lower() function works in ASCII-encoding machines */
	if (i >= 'A' && i <= 'Z')
		return i + 32;
	return i;
}

int Query (const char* hint, const char* validOptions, int ignoreCase) {
	/** Require an input from user. Re-require until they provide a valid one **/
	int optionsCount = strlen (validOptions);
	char tmp_fmt_str[100];
	sprintf (tmp_fmt_str, "%%s(%%%ds)", optionsCount);
	printf (tmp_fmt_str, hint, validOptions);
	char x;
	while (scanf ("%c", &x) != EOF) {
		getchar ();
		int i;
		for (i = 0; i < optionsCount; ++i) {
			if (validOptions[i] == x || (ignoreCase && lower (validOptions[i]) == lower (x)))
				return validOptions[i]; /* Always return what in valid options even if input doesn't equal to valid ones */
		}
	}
	return 0; /* Avoid compiler warning */
}


Stack* create () {
	Stack * sta = malloc (sizeof (Stack));
	if (sta != (Stack *)NULL) {
		sta->heap = NULL;
		sta->cnt = 0;
		sta->buf_size = 0;
	}
	return sta;
}

void release (Stack * st) {
	if (st == NULL)
		return;
	if (st->heap != NULL)
		free (st->heap);
	free (st);
}

int isEmpty (Stack *st) {
	return !(st->cnt);
}

int push (Stack *st, DataType data) {
	if (st == NULL)
		return 0;
	if (st->cnt == st->buf_size)/*heap full*/ {
		if (!_incBuf (st)) {
			puts ("Failed to expand stack!");
			puts ("Operation failed.");
			return -1;
		}
	}
	st->heap[(st->cnt)++] = data;
	return 1;
}

int _incBuf (Stack *st) {
	if (st->heap == NULL) {
		st->heap = malloc (sizeof (DataType) * BUF_INCREMENT);
		st->buf_size = BUF_INCREMENT;
	}
	else {
		/*heap already exists*/
		DataType *newheap = realloc (st->heap, sizeof (DataType) * (st->buf_size + BUF_INCREMENT));
		if (newheap == NULL) {
			puts ("Failed to allocate more memory space!");
			return 0;
		}
		st->heap = newheap;
		(st->buf_size) += BUF_INCREMENT;
	}
	return 1;
}

int pop (Stack *st, DataType *retval) {
	if (st == NULL) {
		return 0;
	}
	if (!(st->cnt)) {
		puts ("Can't pop from an empty stack.");
		return 0;
	}
	if (retval != NULL) {
		*retval = st->heap[--(st->cnt)];
		return 1;
	}
	return 0;
}

