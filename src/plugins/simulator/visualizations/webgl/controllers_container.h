#ifndef LUA_CONTROLLERSS
#define LUA_CONTROLLERSS

#include <argos3/core/wrappers/lua/lua_controller.h>
#include <map>
#include <sstream>
#include <vector>
#include <fstream>
#include "message.h"

namespace argos {
    class CLuaControllers {
    private:
        typedef std::map<std::string, std::vector<CLuaController *>> TMAP;
        TMAP m_mapControllers;

        // Two scripts can have the same name..
        inline std::string FileName(const std::string& strPath) {
            size_t sPos = strPath.find_last_of('/');
            if (sPos == std::string::npos) {
                return strPath;
            } else {
                return strPath.substr(sPos + 1);
            }
        }

    public:
        CLuaControllers() = default;
        ~CLuaControllers() = default;

        void AddController(CLuaController *ctrl) {
            m_mapControllers[ctrl->getScriptName()].push_back(ctrl);
        }

        std::string GetJson() {
            std::ostringstream cJsonStream;
            cJsonStream << R"""(
                {"messageType": "luaScripts", "scripts": [
            )""";
            for (auto k : m_mapControllers) {
                cJsonStream << "{\"name\": \"";
                for (char c: k.first) {
                    EscapeChar(cJsonStream, c);
                }
                cJsonStream << "\", \"content\": \"";
                std::ifstream luaFile;
                luaFile.open(k.first);
                if (!luaFile.is_open()) {
                    THROW_ARGOSEXCEPTION("Can't open lua file");
                }
                char c;
                while (luaFile.get(c)) {
                    EscapeChar(cJsonStream, c);
                }
                cJsonStream << "\"},";
            }
            std::string strLuaFiles = cJsonStream.str();
            // last char is a leading comma
            strLuaFiles[strLuaFiles.length() - 1] = ']';
            strLuaFiles.push_back('}');
            std::cout << "Scripts" << strLuaFiles << std::endl;
            return strLuaFiles;
        }

        bool UpdateScriptContent(const std::string& strScriptFileName, std::string strLuaSource) {
            auto itController = m_mapControllers.find(strScriptFileName);
            if (itController != m_mapControllers.end()) {
                std::ofstream cFile;
                cFile.open(itController->first);
                cFile << strLuaSource;
                cFile.close();
                for (CLuaController* cController: itController->second) {
                    cController->SetLuaScript(itController->first);
                }
                return true;
            }
            return false;
        }
    };
} // namespace argos

#endif