
using eosio::print;

#define EVENTKV(key, value) \
{ \
    print('"'); \
    print(key); \
    print('"'); \
    print(':'); \
    print('"'); \
    print(value); \
    print('"'); \
    print(",");\
} while(0);

#define EVENTKVL(key, value) \
{ \
    print('"'); \
    print(key); \
    print('"'); \
    print(':'); \
    print('"'); \
    print(value); \
    print('"'); \
} while(0);

#define START_EVENT(etype, version) \
{ \
    print("{"); \
    EVENTKV("version",version) \
    EVENTKV("etype",etype) \
}

#define END_EVENT() \
    print("}\n");
