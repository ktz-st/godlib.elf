#include "reflect.h"

#include <godlib/string/string.h>
#include <godlib/string/strpath.h>
#include <errno.h>
#include <float.h>

sReflectType gReflectTypesFundamental[] =
{
    { "S8", sizeof(S8), eReflectType_Leaf | eReflectType_Signed, 0, 0, 0 },
    { "S16", sizeof(S16), eReflectType_Leaf | eReflectType_Signed, 0, 0, 0 },
    { "S32", sizeof(S32), eReflectType_Leaf | eReflectType_Signed, 0, 0, 0 },
    { "U8", sizeof(U8), eReflectType_Leaf, 0, 0, 0 },
    { "U16", sizeof(U16), eReflectType_Leaf, 0, 0, 0 },
    { "U32", sizeof(U32), eReflectType_Leaf, 0, 0, 0 },
    { "F32", sizeof(F32), eReflectType_Leaf | eReflectType_Float, 0, 0, 0 },
    { "string", sizeof(sString), eReflectType_Leaf | eReflectType_String, 0, 0, 0 },
};

sReflectDictionary gReflectFundamentalDictionary =
    { mARRAY_COUNT(gReflectTypesFundamental), gReflectTypesFundamental, 0, 0 };

sReflectType * Reflect_GetpType(const sReflectDictionary * apDictionary, const char * apType)
{
    U32 i;
    if (!apDictionary || !apDictionary->mpTypes || !apType)
        return 0;
    for (i = 0; i < apDictionary->mTypeCount; ++i)
        if (apDictionary->mpTypes[i].mpTypeName &&
            !String_StrCmp(apDictionary->mpTypes[i].mpTypeName, apType))
            return &apDictionary->mpTypes[i];
    return 0;
}

U8 * Reflect_GetpData(const sReflectType * apType, U8 * apData, const char * apElement)
{
    U16 i;
    if (!apType || !apType->mpElements || !apData || !apElement)
        return 0;
    for (i = 0; i < apType->mElementCount; ++i)
    {
        const sReflectElement * element = &apType->mpElements[i];
        if (element->mpElementName && !String_StrCmp(apElement, element->mpElementName))
            return element->mOffset < apType->mSizeBytes ? apData + element->mOffset : 0;
    }
    return 0;
}

/* Keep ASCII whitespace independent of the toolchain's ctype table: the
 * Atari library is also linked by libcmini applications. */
static U8 Reflect_IsSpace(char c)
{
    return c == ' ' || (c >= '\t' && c <= '\r');
}

/* libcmini's strtoul does not report overflow through errno. Check the
 * magnitude before each decimal digit, independently of the C library. */
static U8 Reflect_Integer(const char * text, U8 isSigned, U32 * magnitude, U8 * negative)
{
    U32 value = 0, limit;
    U16 digits = 0;
    while (Reflect_IsSpace(*text)) ++text;
    *negative = *text == '-';
    if (*text == '-' || *text == '+') ++text;
    if (*negative && !isSigned) return 0;
    limit = isSigned ? (*negative ? 2147483648UL : 2147483647UL) : 4294967295UL;
    while (*text >= '0' && *text <= '9')
    {
        U32 digit = (U32)(*text++ - '0');
        if (value > (limit - digit) / 10UL) return 0;
        value = value * 10UL + digit;
        ++digits;
    }
    while (Reflect_IsSpace(*text)) ++text;
    if (!digits || *text) return 0;
    *magnitude = value;
    return 1;
}

static U8 Reflect_Float(const char * text, F32 * result)
{
    const char * begin;
    U16 digits = 0;
    double value;
    while (Reflect_IsSpace(*text)) ++text;
    begin = text;
    if (*text == '+' || *text == '-') ++text;
    while (*text >= '0' && *text <= '9') { ++digits; ++text; }
    if (*text == '.')
    {
        ++text;
        while (*text >= '0' && *text <= '9') { ++digits; ++text; }
    }
    if (!digits) return 0;
    if (*text == 'e' || *text == 'E')
    {
        ++text; digits = 0;
        if (*text == '+' || *text == '-') ++text;
        while (*text >= '0' && *text <= '9') { ++digits; ++text; }
        if (!digits) return 0;
    }
    while (Reflect_IsSpace(*text)) ++text;
    if (*text) return 0;
    /* Validate the whole token ourselves: libcmini's exponent end pointer is
     * unreliable. Convert via double to avoid its strtof rounding shortcuts. */
    errno = 0;
    value = strtod(begin, 0);
    if (errno == ERANGE || !(value <= FLT_MAX && value >= -FLT_MAX)) return 0;
    *result = (F32)value;
    return 1;
}

U8 Reflect_SetData(const sReflectType * apType, const sString * apElementName,
    const sString * apValue, void * apStructBase)
{
    U16 i;
    if (!apType || !apType->mpElements || !apElementName || !apElementName->mpChars ||
        !apValue || (!apValue->mpChars && String_GetLength(apValue)) || !apStructBase)
        return 0;
    for (i = 0; i < apType->mElementCount; ++i)
    {
        const sReflectElement * element = &apType->mpElements[i];
        const sReflectType * type;
        U8 * dst;
        sStringPath chars;
        char * end;
        if (!element->mpElementName || !String_IsEqualNT(apElementName, element->mpElementName))
            continue;
        type = Reflect_GetpType(&gReflectFundamentalDictionary, element->mpTypeName);
        if (!type || !(type->mTypeFlags & eReflectType_Leaf) ||
            element->mOffset > apType->mSizeBytes ||
            type->mSizeBytes > apType->mSizeBytes - element->mOffset)
            return 0;
        dst = (U8 *)apStructBase + element->mOffset;
        if (type->mTypeFlags & eReflectType_String)
        {
            /* Preserve the existing shallow string-descriptor assignment. */
            *(sString *)dst = *apValue;
            return 1;
        }
        /* JSON tokens are bounded views, not necessarily zero terminated.
         * Reject overlong tokens instead of silently parsing a truncated value. */
        if (!String_GetLength(apValue) || String_GetLength(apValue) >= sizeof(chars.mChars))
            return 0;
        StringPath_SetSS(&chars, apValue);
        /* Embedded zero bytes must not turn a malformed token into a prefix. */
        for (end = chars.mChars; end < chars.mChars + String_GetLength(apValue); ++end)
            if (!*end)
                return 0;
        if (type->mTypeFlags & eReflectType_Float)
        {
            F32 value;
            if (!Reflect_Float(chars.mChars, &value)) return 0;
            *(F32 *)dst = value;
        }
        else
        {
            U32 magnitude;
            U8 negative = 0;
            if (!String_StrCmpi("true", chars.mChars)) magnitude = 1;
            else if (!String_StrCmpi("false", chars.mChars)) magnitude = 0;
            else if (!Reflect_Integer(chars.mChars,
                (U8)(type->mTypeFlags & eReflectType_Signed), &magnitude, &negative)) return 0;
            if (type->mTypeFlags & eReflectType_Signed)
            {
                S32 value = negative ? (magnitude == 2147483648UL ?
                    (-2147483647L - 1L) : -(S32)magnitude) : (S32)magnitude;
                if (type->mSizeBytes == 1 && value >= -128L && value <= 127L)
                    *(S8 *)dst = (S8)value;
                else if (type->mSizeBytes == 2 && value >= -32768L && value <= 32767L)
                    *(S16 *)dst = (S16)value;
                else if (type->mSizeBytes == 4)
                    *(S32 *)dst = value;
                else return 0;
            }
            else
            {
                if (type->mSizeBytes == 1 && magnitude <= 255UL)
                    *(U8 *)dst = (U8)magnitude;
                else if (type->mSizeBytes == 2 && magnitude <= 65535UL)
                    *(U16 *)dst = (U16)magnitude;
                else if (type->mSizeBytes == 4)
                    *(U32 *)dst = magnitude;
                else return 0;
            }
        }
        return 1;
    }
    return 0;
}
