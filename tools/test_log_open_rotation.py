"""Compile the production log opener with config/logger doubles."""
import os
import pathlib
import shutil
import subprocess
import tempfile
import unittest


class LogOpenRotationTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which('g++'), 'g++ unavailable')
    def test_write_append_disabled_and_failure_paths(self):
        root = pathlib.Path(__file__).resolve().parents[1]
        source = pathlib.Path(os.environ.get('TBOTS_TEST_SOURCE', root / 'ai/playerbot/PlayerbotAIConfig.cpp')).read_text()
        body = source[source.index('bool PlayerbotAIConfig::openLog('):source.index('void PlayerbotAIConfig::log(')]
        fixture = r'''
#include <cassert>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iterator>
#include <unordered_map>
#include "LogFileRotation.h"
using namespace std;
struct Config { string dir; string GetStringDefault(char const*, char const*) { return dir; } } sConfig;
struct Log { unsigned errors=0; void outError(char const*, ...) { ++errors; } } sLog;
class PlayerbotAIConfig {
public:
 bool enabled=true;
 unordered_map<string,pair<FILE*,bool>> logFiles;
 ai::LogFileRotation logRotation;
 bool hasLog(string const&) { return enabled; }
 bool openLog(string, char const*, bool haslog=false);
 ~PlayerbotAIConfig() { for(auto const& entry: logFiles) if(entry.second.second) fclose(entry.second.first); }
};
''' + body + r'''
static void Write(string const& p, char const* text) { ofstream(p) << text; }
static string Read(string const& p) { ifstream in(p); return {istreambuf_iterator<char>(in),istreambuf_iterator<char>()}; }
int main(int argc, char** argv) {
 assert(argc==2); sConfig.dir=argv[1]; string base=sConfig.dir+"/";
 string path=base+"bot_events.csv";
 Write(path,"prior run\n"); Write(path+".1","older run\n");
 {
  PlayerbotAIConfig cfg; cfg.enabled=false;
  assert(!cfg.openLog("bot_events.csv","w"));
  assert(Read(path)=="prior run\n" && Read(path+".1")=="older run\n");
  cfg.enabled=true; assert(cfg.openLog("bot_events.csv","w"));
  assert(Read(path).empty() && Read(path+".1")=="prior run\n");
  fputs("current run\n",cfg.logFiles.at("bot_events.csv").first);
  assert(cfg.openLog("bot_events.csv","w")); // Closes/flushes previous stream.
  assert(Read(path).empty() && Read(path+".1")=="prior run\n");
  fputs("refreshed\n",cfg.logFiles.at("bot_events.csv").first);
 }
 {
  PlayerbotAIConfig cfg; assert(cfg.openLog("bot_events.csv","w"));
  assert(Read(path+".1")=="refreshed\n");
 }
 string blocked=base+"blocked.csv"; Write(blocked,"keep me\n");
 filesystem::create_directory(blocked+".1"); Write(blocked+".1/previous","keep too\n");
 {
  PlayerbotAIConfig cfg;
  assert(!cfg.openLog("blocked.csv","w"));
  assert(!cfg.logFiles.at("blocked.csv").first && !cfg.logFiles.at("blocked.csv").second);
  assert(Read(blocked)=="keep me\n" && sLog.errors==1);
  assert(cfg.openLog("blocked.csv","a"));
  fputs("appended\n",cfg.logFiles.at("blocked.csv").first);
 }
 assert(Read(blocked)=="keep me\nappended\n");
 {
  PlayerbotAIConfig cfg; cfg.enabled=false;
  Write(base+"forced.log","forced previous\n");
  assert(cfg.openLog("forced.log","w",true));
  assert(Read(base+"forced.log.1")=="forced previous\n");
  cfg.enabled=true;
  assert(!cfg.openLog("missing/subdir.csv","w"));
  assert(!cfg.logFiles.at("missing/subdir.csv").first && !cfg.logFiles.at("missing/subdir.csv").second);
 }
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            cpp = pathlib.Path(tmp) / 'test.cpp'
            exe = pathlib.Path(tmp) / 'test'
            cpp.write_text(fixture)
            subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-I',
                            str(root / 'ai/playerbot'), str(cpp), '-o', str(exe)], check=True)
            subprocess.run([str(exe), tmp], check=True)


if __name__ == '__main__':
    unittest.main()
