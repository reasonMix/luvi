
extern "C" {
  #include "lua.h"
  #include "lauxlib.h"
  #include "../deps/xxtea.h"
}
#include <stdio.h>

//#include <io.h>
//#include <"sys/types.h">
//#include<"sys/stat.h">


#include <string>
using namespace std;

extern "C" void addLuaLoader(lua_CFunction func,lua_State* _state)
{
    printf("call addLuaLoader \n");
    if (!func) return;

    // stack content after the invoking of the function
    // get loader table
    lua_getglobal(_state, "package");                                  /* L: package */
    lua_getfield(_state, -1, "loaders");                               /* L: package, loaders */

    // insert loader into index 2
    lua_pushcfunction(_state, func);                                   /* L: package, loaders, func */
    for (int i = (int)(lua_objlen(_state, -2) + 1); i > 2; --i)
    {
        lua_rawgeti(_state, -2, i - 1);                                /* L: package, loaders, func, function */
        // we call lua_rawgeti, so the loader table now is at -3
        lua_rawseti(_state, -3, i);                                    /* L: package, loaders, func */
    }
    lua_rawseti(_state, -2, 2);                                        /* L: package, loaders */

    // set loaders into package
    lua_setfield(_state, -2, "loaders");                               /* L: package */

    lua_pop(_state, 1);
}


static bool isEncrypted(std::string sign_,const char* data,int dataLen)
{
    return dataLen >= sign_.size() && memcmp(sign_.data(), data, sign_.size()) == 0;
}

static char* decrypt(char* data,std::string sign_,int dataLen,std::string key_,xxtea_long* len)
{

    char* buf = (char*)xxtea_decrypt((unsigned char*)(data + sign_.size()), (xxtea_long)(dataLen-sign_.size()),
        (unsigned char*)(&key_[0]), (xxtea_long) key_.size(), len);
    //printf("decrypt len is %d\n", len);
    //printf("decrypt data is %s\n", buf);

    return buf;
}


static char* readFile(char* name,char* mode,ssize_t* size) {
  FILE *pFile=fopen((const char*)name,(const char*)mode);
  char *pBuf;
  fseek(pFile,0,SEEK_END);
  int len = ftell(pFile);
  pBuf = (char*)malloc(len + 1);
  rewind(pFile);
  fread(pBuf,1,len,pFile);
  pBuf[len] = 0;
  fclose(pFile);
  *size = len+1;

  if (isEncrypted("renhualiu",pBuf,len)) {
    printf("file is encryped %s\n", name);
    xxtea_long dlen;
    char* ret = decrypt(pBuf,"renhualiu",len,"GamesCity",&dlen);
    free(pBuf);
    pBuf = ret;
    *size = dlen;
  }

  return pBuf;
}

static int fileExist(const char* filename) {
  FILE *pFile=fopen((const char*)filename,(const char*)"r");
  int ret = 0;
  if (pFile != NULL){
    ret = 1;
    fclose(pFile);
  }

  return ret;
}

static int cocos2dx_lua_loader(lua_State *L)
{
    static const char* NOT_BYTECODE_FILE_EXT = ".lua";

    std::string filename(luaL_checkstring(L, 1));
    size_t pos = filename.rfind(NOT_BYTECODE_FILE_EXT);
    if (pos != std::string::npos)
    {
        filename = filename.substr(0, pos);
    }

    pos = filename.find_first_of(".");
    while (pos != std::string::npos)
    {
        filename.replace(pos, 1, "/");
        pos = filename.find_first_of(".");
    }

    //printf("load lua %s\n", filename.c_str());

    // search file in package.path
    unsigned char* chunk = nullptr;
    ssize_t chunkSize = 0;
    std::string chunkName;

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path");
    std::string searchpath(lua_tostring(L, -1));
    lua_pop(L, 1);
    size_t begin = 0;
    size_t next = searchpath.find_first_of(";", 0);

    do
    {
        if (next == std::string::npos)
            next = searchpath.length();
        std::string prefix = searchpath.substr(begin, next);
        if (prefix[0] == '.' && prefix[1] == '/')
        {
            prefix = prefix.substr(2);
        }

        pos = prefix.find("?.lua");
        chunkName = prefix.substr(0, pos) + filename + NOT_BYTECODE_FILE_EXT;
        if (1 == fileExist(chunkName.c_str()))//(utils->isFileExist(chunkName))
        {
            //logIfUpdated(luaL_checkstring(L, 1), utils->fullPathForFilename(chunkName));
            //chunk = utils->getFileData(chunkName.c_str(), "rb", &chunkSize);
            chunk = (unsigned char*)readFile((char*)(chunkName.c_str()),(char*)"rb",&chunkSize);

            break;
        }

        begin = next + 1;
        next = searchpath.find_first_of(";", begin);
    } while (begin < (int)searchpath.length());

    if (chunk)
    {
        //LuaStack* stack = LuaEngine::getInstance()->getLuaStack();
        //CCLOG("%s:\n%s", chunkName.c_str(), chunk);
        //stack->luaLoadBuffer(L, (char*)chunk, (int)chunkSize, chunkName.c_str());
        luaL_loadbuffer(L, (const char*)chunk, chunkSize,(const char*)chunkName.c_str());
        free(chunk);
    }
    else
    {
        return 0;
    }

    return 1;
}


extern "C" void initCustomLoader(lua_State* _state) {
  addLuaLoader(cocos2dx_lua_loader,_state);
}
