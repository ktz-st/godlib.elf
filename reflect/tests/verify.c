#include <godlib/reflect/reflect.h>
#include <godlib/string/string.h>
#include <godlib/memory/memory.h>
#include <godlib/file/file.h>
#include <godlib/lexer/json.h>
#include <string.h>

/* Use the actual FED descriptor too, not a corrected copy in the test. */
extern sReflectType gFedJSON_Layout_Type;
extern sReflectDictionary gReflectFundamentalDictionary;

typedef struct sValues
{
    U32 guard0;
    S8 s8;
    U8 u8;
    S16 s16;
    U16 u16;
    S32 s32;
    U32 u32;
    F32 f32;
    sString text;
    U32 guard1;
} sValues;

#define FIELD(t, n) { t, #n, offsetof(sValues, n), 0 }
static sReflectElement elements[] = {
    FIELD("S8", s8), FIELD("U8", u8), FIELD("S16", s16), FIELD("U16", u16),
    FIELD("S32", s32), FIELD("U32", u32), FIELD("F32", f32), FIELD("string", text)
};
static sReflectType type = { "values", sizeof(sValues), 0, mARRAY_COUNT(elements), elements, 0 };
static U32 result[68] = { 0x52464c54UL, 0, 0, 0 };
#define CHECK(x) do { ++result[1]; if (!(x)) { if (result[2] < 64) result[4 + result[2]] = __LINE__; ++result[2]; if (!result[3]) result[3] = __LINE__; } } while (0)

static U8 Set(const char *name, const char *value, sValues *dst)
{
    sString key, token;
    String_SetStaticNT(&key, name);
    String_SetStaticNT(&token, value);
    return Reflect_SetData(&type, &key, &token, dst);
}

static void Reject(const char *name, const char *value)
{
    sValues dst, before;
    memset(&dst, 0xa5, sizeof(dst)); before = dst;
    CHECK(!Set(name, value, &dst));
    CHECK(!memcmp(&dst, &before, sizeof(dst)));
}

/* Compare every byte, including guards and unrelated fields after each write. */
#define SET(n, token, v) do { \
    memset(&dst, 0xa5, sizeof(dst)); expected = dst; expected.n = (v); \
    CHECK(Set(#n, token, &dst)); CHECK(!memcmp(&dst, &expected, sizeof(dst))); \
    CHECK(Reflect_GetpData(&type, (U8 *)&dst, #n) == (U8 *)&dst.n); \
} while (0)

S16 GodLib_Game_Main(S16 argc, char **argv)
{
    sValues dst, expected;
    sString key, token, source;
    sElementCollectionJSON collection;
    U32 index;
    sFileHandle file;
    char bounded[] = { '7', '0', '0', '0', '0', '9' };
    char longToken[260];
    char embedded[] = { '1', 0, '2' };
    sReflectElement bad = { "U32", "bad", sizeof(sValues) - 2, 0 };
    sReflectType badType = { "bad", sizeof(sValues), 0, 1, &bad, 0 };
    struct { S32 x0, y0, x1, y1; } layout;
    (void)argc; (void)argv;
    Memory_Init();

    SET(s8, "-128", -128); SET(s8, "127", 127);
    SET(u8, "0", 0); SET(u8, "255", 255);
    SET(s16, "-32768", -32768); SET(s16, "32767", 32767);
    SET(u16, "0", 0); SET(u16, "65535", 65535);
    SET(s32, "-2147483648", (-2147483647L - 1L)); SET(s32, "2147483647", 2147483647L);
    SET(s32, "70000", 70000L); SET(s32, "-70000", -70000L);
    SET(u32, "4294967295", 4294967295UL); SET(u32, "2147483648", 2147483648UL);
    SET(u32, "0", 0); SET(u32, "  +70000  ", 70000UL);
    SET(s8, "TrUe", 1); SET(s16, "false", 0); SET(u32, "TRUE", 1);
    SET(f32, "-12.5", -12.5f); SET(f32, "1.25e2", 125.0f);
    SET(f32, "1.25e+2", 125.0f); SET(f32, "-1.25E-1", -0.125f);
    SET(f32, "0.03125", 0.03125f); SET(f32, "  +2.5  ", 2.5f);

    memset(&dst, 0xa5, sizeof(dst)); expected = dst;
    String_SetStaticNT(&key, "s32");
    String_SetStatic(&token, bounded, 5); /* Not zero terminated; trailing 9 is outside the view. */
    expected.s32 = 70000L;
    CHECK(Reflect_SetData(&type, &key, &token, &dst));
    CHECK(!memcmp(&dst, &expected, sizeof(dst)));
    CHECK(bounded[5] == '9');
    String_SetStaticNT(&key, "text"); String_SetStatic(&token, bounded, 5);
    expected.text = token;
    CHECK(Reflect_SetData(&type, &key, &token, &dst));
    CHECK(!memcmp(&dst, &expected, sizeof(dst)));

    Reject("s8", "128"); Reject("s8", "-129"); Reject("u8", "256");
    Reject("s16", "32768"); Reject("s16", "-32769"); Reject("u16", "65536");
    Reject("s32", "2147483648"); Reject("s32", "-2147483649");
    Reject("u32", "4294967296"); Reject("u32", "-1"); Reject("u32", " -1");
    Reject("s32", ""); Reject("s32", "abc"); Reject("s32", "12x");
    Reject("s32", "1.5"); Reject("s32", " "); Reject("missing", "1");
    Reject("f32", "nan"); Reject("f32", "inf"); Reject("f32", "1e999");
    Reject("f32", "2.5x"); Reject("f32", "");
    Reject("f32", "1e"); Reject("f32", "1e+"); Reject("f32", "1e-"); Reject("f32", ".");
    memset(longToken, '1', sizeof(longToken)); longToken[259] = 0;
    Reject("u32", longToken);

    String_SetStaticNT(&key, "s32"); String_SetStatic(&token, embedded, sizeof(embedded));
    expected = dst;
    CHECK(!Reflect_SetData(&type, &key, &token, &dst));
    CHECK(!memcmp(&dst, &expected, sizeof(dst)));
    CHECK(!Reflect_GetpData(&type, (U8 *)&dst, "U32")); /* Type name is not a field name. */
    CHECK(!Reflect_GetpData(&type, (U8 *)&dst, "missing"));
    CHECK(!Reflect_GetpData(0, (U8 *)&dst, "s32"));
    CHECK(!Reflect_GetpType(0, "U32"));
    CHECK(!Reflect_GetpType(&gReflectFundamentalDictionary, 0));
    CHECK(Reflect_GetpType(&gReflectFundamentalDictionary, "F32")->mTypeFlags & eReflectType_Leaf);
    String_SetStaticNT(&key, "bad"); String_SetStaticNT(&token, "123");
    CHECK(!Reflect_SetData(&badType, &key, &token, &dst));
    CHECK(!memcmp(&dst, &expected, sizeof(dst)));
    bad.mOffset = sizeof(dst); CHECK(!Reflect_GetpData(&badType, (U8 *)&dst, "bad"));
    CHECK(!Reflect_SetData(0, &key, &token, &dst));
    CHECK(!Reflect_SetData(&type, 0, &token, &dst));
    CHECK(!Reflect_SetData(&type, &key, 0, &dst));
    CHECK(!Reflect_SetData(&type, &key, &token, 0));

    /* Exercise the real JSON lexer/token views, not just manufactured sStrings. */
    String_SetStaticNT(&source, "{\"s32\":-2147483648,\"u32\":4294967295,\"f32\":-12.5,\"u8\":true,\"text\":\"moon\"}");
    memset(&dst, 0xa5, sizeof(dst)); expected = dst;
    JSON_Parse(&source, &collection); JSON_ElementsTrim(&collection); index = 0;
    JSON_ElementsToStruct(&collection, &type, &index, &dst);
    CHECK(dst.s32 == (-2147483647L - 1L)); CHECK(dst.u32 == 4294967295UL);
    CHECK(dst.f32 == -12.5f); CHECK(dst.u8 == 1);
    CHECK(String_IsEqualNT(&dst.text, "moon"));
    CHECK(dst.guard0 == expected.guard0 && dst.guard1 == expected.guard1);
    JSON_Destroy(&collection);

    String_SetStaticNT(&source, "{\"f32\":3.125e-2,\"text\":\"a-b+c.d\"}");
    JSON_Parse(&source, &collection); JSON_ElementsTrim(&collection); index = 0;
    JSON_ElementsToStruct(&collection, &type, &index, &dst);
    CHECK(dst.f32 == 0.03125f); CHECK(String_IsEqualNT(&dst.text, "a-b+c.d"));
    JSON_Destroy(&collection);

    String_SetStaticNT(&source, "{\"x0\":70000,\"y0\":-70000,\"x1\":320,\"y1\":200}");
    JSON_Parse(&source, &collection); JSON_ElementsTrim(&collection); index = 0;
    memset(&layout, 0xa5, sizeof(layout));
    CHECK(gFedJSON_Layout_Type.mElementCount == 4);
    JSON_ElementsToStruct(&collection, &gFedJSON_Layout_Type, &index, &layout);
    CHECK(layout.x0 == 70000L && layout.y0 == -70000L && layout.x1 == 320 && layout.y1 == 200);
    JSON_Destroy(&collection);

    file = File_Create("RESULT.BIN");
    if (File_HandleIsValid(file)) { File_Write(file, sizeof(result), result); File_Close(file); }
    Memory_DeInit();
    return (S16)result[2];
}
