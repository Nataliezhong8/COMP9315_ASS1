/*
 * pname.c
 * author1: Yinghong Zhong(z5233608)
 * author2: Shaowei Ma(z5238010)
 * version:1.3
 * course: COMP9315
 * Item: Assignment1
*/

#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>

#include "postgres.h"
#include "fmgr.h"
#include "libpq/pqformat.h"		/* needed for send/recv functions */
#include "c.h"  // include felxable memory size

PG_MODULE_MAGIC;

typedef struct pName
{
    int length;
	// can I use poniter here
	// or familyName[] ?
	char familyName[FLEXIBLE_ARRAY_MEMBER];
	char givenName[FLEXIBLE_ARRAY_MEMBER];
} pName;

static int checkName(char *name);

static int compareNames(pName *a, pName *b);

static bool checkName(char *name){
	int cflags = REG_EXTENDED;
	regex_t reg;
	bool ret = true;

	const char *pattern = "^[A-Z]([A-Za-z]|[-|'])+([ ][A-Z]([A-Za-z]|[-|'])+)*,[ ]?[A-Z]([A-Za-z]|[-|'])+([ ][A-Z]([A-Za-z]|[-|'])+)*$";
	regcomp(&reg, pattern, cflags);
	
	int status = regexec(&reg, name, 0, NULL, 0);
	
	if (status != 0){
		ret = false;
	}
	regfree(&regex);
	return ret;
}

// remove the first space of a name if its first char is a space
static char *removeFirstSpace(char *name){
	char *new_name = name;
	if (*name == ' '){
		new_name++;
	}
	return new_name;
}

/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/

PG_FUNCTION_INFO_V1(pname_in);

Datum
pname_in(PG_FUNCTION_ARGS)
{
	char *NameIn = PG_GETARG_CSTRING(0);
	char *familyName, 
		 *givenName;

	if (checkName(NameIn) == false){
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input syntax for type pname: \"%s\"", NameIn)));
	}

	// scan the input string and set familyName, givenName
	sscanf(NameIn, "%s,%s", familyName, givenName);

	// check if the given name start with a space
	givenName = removeFirstSpace(givenName);

	// set new pname object's size , +2 for 2"\0"
	int32 new_pname_size = strlen(familyName) + strlen(givenName) + 2;

	// assign memory to result
	pName *result = (pName *)palloc(VARHDRSZ + new_pname_size);

	// set VARSIZE
	SET_VARSIZE(result, VARHDRSZ + new_pname_size);

	// memcopy
	memcpy(VARDATA(result), VARDATA_ANY(familyName), (strlen(familyName) + 1));
	memcpy(VARDATA(result) + strlen(familyName) + 1, VARDATA_ANY(givenName), (strlen(givenName) + 1));

	// locate pointer
	result->familyName = result + VARHDRSZ；
	result->givenName = result->familyName + strlen(familyName) + 1；
	
	// copy familyName and givenName to result
	//strcpy(result->familyName, familyName);
	//strcpy(result->givenName, givenName);
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(pname_out);

Datum
pname_out(PG_FUNCTION_ARGS)
{
	pName *fullname = (pName *) PG_GETARG_POINTER(0);
	char  *result;

	result = psprintf("(%s,%s)", fullname->familyName, fullname->givenName);
	PG_RETURN_CSTRING(result);
}



/*****************************************************************************
 * Operator class for defining B-tree index
 *
 * It's essential that the comparison operators and support function for a
 * B-tree index opclass always agree on the relative ordering of any two
 * data values.  Experience has shown that it's depressingly easy to write
 * unintentionally inconsistent functions.  One way to reduce the odds of
 * making a mistake is to make all the functions simple wrappers around
 * an internal three-way-comparison function, as we do here.
 *****************************************************************************/

#define Mag(c)	((c)->x*(c)->x + (c)->y*(c)->y)

static int
complex_abs_cmp_internal(Complex * a, Complex * b)
{
	double		amag = Mag(a),
				bmag = Mag(b);

	if (amag < bmag)
		return -1;
	if (amag > bmag)
		return 1;
	return 0;
}


PG_FUNCTION_INFO_V1(complex_abs_lt);

Datum
complex_abs_lt(PG_FUNCTION_ARGS)
{
	Complex    *a = (Complex *) PG_GETARG_POINTER(0);
	Complex    *b = (Complex *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(complex_abs_cmp_internal(a, b) < 0);
}

PG_FUNCTION_INFO_V1(complex_abs_le);

Datum
complex_abs_le(PG_FUNCTION_ARGS)
{
	Complex    *a = (Complex *) PG_GETARG_POINTER(0);
	Complex    *b = (Complex *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(complex_abs_cmp_internal(a, b) <= 0);
}

PG_FUNCTION_INFO_V1(complex_abs_eq);

Datum
complex_abs_eq(PG_FUNCTION_ARGS)
{
	Complex    *a = (Complex *) PG_GETARG_POINTER(0);
	Complex    *b = (Complex *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(complex_abs_cmp_internal(a, b) == 0);
}

PG_FUNCTION_INFO_V1(complex_abs_ge);

Datum
complex_abs_ge(PG_FUNCTION_ARGS)
{
	Complex    *a = (Complex *) PG_GETARG_POINTER(0);
	Complex    *b = (Complex *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(complex_abs_cmp_internal(a, b) >= 0);
}

PG_FUNCTION_INFO_V1(complex_abs_gt);

Datum
complex_abs_gt(PG_FUNCTION_ARGS)
{
	Complex    *a = (Complex *) PG_GETARG_POINTER(0);
	Complex    *b = (Complex *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(complex_abs_cmp_internal(a, b) > 0);
}

PG_FUNCTION_INFO_V1(complex_abs_cmp);

Datum
complex_abs_cmp(PG_FUNCTION_ARGS)
{
	Complex    *a = (Complex *) PG_GETARG_POINTER(0);
	Complex    *b = (Complex *) PG_GETARG_POINTER(1);

	PG_RETURN_INT32(complex_abs_cmp_internal(a, b));
}
