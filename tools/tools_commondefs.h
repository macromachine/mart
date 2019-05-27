
#ifndef __MART_SEMU_GENMU_tools_tools_commondefs__
#define __MART_SEMU_GENMU_tools_tools_commondefs__

#include <libgen.h> //dirname
#include <string>

#include "llvm/Support/FileSystem.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Path.h"

#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)

static const std::string mutantsInfosFileName("mutantsInfos.json");
static const std::string equivalentduplicate_mutantsInfosFileName("equidup-mutantsInfos.json");
static const char *wmOutIRFileSuffix = ".WM.bc";
static const char *covOutIRFileSuffix = ".COV.bc";
static const char *preTCEMetaIRFileSuffix = ".preTCE.MetaMu.bc";
static const char *commonIRSuffix = ".bc";
static const char *metaMuIRFileSuffix = ".MetaMu.bc";
static const char *usefulFolderName = "useful";
#ifdef MART_SEMU_GENMU_OBJECTFILE
static const char *metaMuObjFileSuffix = ".MetaMu.o";
#endif

void printVersion() {
  llvm::outs() << "\nMart (Framework for Multi-Programming Language Mutation "
                  "Testing based on LLVM)\n";
  llvm::outs() << "\t" << TOOLNAME << " 1.0\n";
  llvm::outs() << "\nLLVM (http://llvm.org/):\n";
  llvm::outs() << "\tLLVM version " << LLVM_VERSION_MAJOR << "."
               << LLVM_VERSION_MINOR << "." << LLVM_VERSION_PATCH << "\n";
  llvm::outs() << "\tLLVM tools dir: " << STRINGIFY(LLVM_TOOLS_BINARY_DIR) << "\n";
  llvm::outs() << "\n";
}

std::string getUsefulAbsPath(char *argv0) {
  std::string useful_conf_dir;

  if (false) {
    char *tmpStr = nullptr;
    tmpStr = new char[1 + std::strlen(argv0)]; // Alocate tmpStr1
    std::strcpy(tmpStr, argv0);
    useful_conf_dir.assign(dirname(tmpStr)); 
    delete[] tmpStr;
    tmpStr = nullptr; // del tmpStr1
  } else {
    void *MainExecAddr = (void *)(intptr_t)getUsefulAbsPath;
    llvm::SmallString<512> rootpath(llvm::sys::fs::getMainExecutable(argv0, MainExecAddr));
    assert(!rootpath.empty() && "Failed to get xecutable abspath!");
    llvm::sys::path::remove_filename(rootpath);
    useful_conf_dir = rootpath.str();
  }

  useful_conf_dir = useful_conf_dir + "/" + usefulFolderName + "/";
  return useful_conf_dir;
}

#endif
