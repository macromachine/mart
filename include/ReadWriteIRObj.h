
#include <string>
//#include <fstream>
#include <system_error> //error_code

#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
//#include <llvm/Support/raw_os_ostream.h>
#include "llvm/Support/FileSystem.h"    //for F_None

#include "llvm/Support/MemoryBuffer.h"

#ifdef KLEE_SEMU_GENMU_OBJECTFILE
#if (LLVM_VERSION_MAJOR <= 3) && (LLVM_VERSION_MINOR < 5)
#include "llvm/PassManager.h"
#else
#include "llvm/IR/LegacyPassManager.h"
#endif

#include "llvm/ADT/Optional.h"  //for Optional
#include "llvm/Support/Host.h"   //for getDefaultTargetTriple
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#endif  //#ifdef KLEE_SEMU_GENMU_OBJECTFILE

#include "llvm/Transforms/Utils/Cloning.h"  //for CloneModule

class ReadWriteIRObj
{
    std::unique_ptr<llvm::MemoryBuffer> mBuf;
public:
    ReadWriteIRObj(): mBuf(nullptr) {}
    void setToModule(llvm::Module const *module)
    {
        std::string data;
        llvm::raw_string_ostream OS(data);
        llvm::WriteBitcodeToFile(module, OS);
        mBuf = llvm::MemoryBuffer::getMemBufferCopy(OS.str());
    }
    
    ReadWriteIRObj(ReadWriteIRObj const &cp)
    {   
        mBuf = llvm::MemoryBuffer::getMemBuffer(cp.mBuf->getMemBufferRef());
    }
    
    inline llvm::Module * readIR ()// (std::unique_ptr<llvm::Module> &module)
    {
        llvm::SMDiagnostic SMD;
#if (LLVM_VERSION_MAJOR <= 3) && (LLVM_VERSION_MINOR < 5)
        return (llvm::ParseIR(*mBuf, SMD, llvm::getGlobalContext()));
#else
        return llvm::parseIR(*mBuf, SMD, llvm::getGlobalContext()).release();
#endif
    }
    
    static inline llvm::Module * cloneModuleAndRelease (llvm::Module *M)
    {
#if (LLVM_VERSION_MAJOR <= 3) && (LLVM_VERSION_MINOR < 5)
        return llvm::CloneModule(M);
#else
       return llvm::CloneModule(M).release();
#endif
    }
    
    static bool readIR (const std::string filename, std::unique_ptr<llvm::Module> &module)
    {
        llvm::SMDiagnostic SMD;
#if (LLVM_VERSION_MAJOR <= 3) && (LLVM_VERSION_MINOR < 5)
        module.reset(llvm::ParseIRFile(filename, SMD, llvm::getGlobalContext()));
#else
        module = llvm::parseIRFile(filename, SMD, llvm::getGlobalContext());
#endif

        if (!module) 
        {
            llvm::errs() << "Failed parsing '" << filename << "' file:\n";
            SMD.print("MuLL", llvm::errs());
            return false;
        }
        return true;
    }

    static bool writeIR (const llvm::Module * module, const std::string filename)
    {
#if (LLVM_VERSION_MAJOR <= 3) && (LLVM_VERSION_MINOR < 5)
        std::string estr("");
        llvm::raw_fd_ostream Out(filename.c_str(), estr, llvm::sys::fs::F_None);
        if (estr.length() > 0) 
        {
            llvm::errs() << "Could not open file: " << estr;
            return false;
        }
#else
        std::error_code EC;
        llvm::raw_fd_ostream Out(filename, EC, llvm::sys::fs::F_None);
        if (EC) 
        {
            llvm::errs() << "Could not open file: " << EC.message();
            return false;
        }
#endif
        //std::ofstream ofstr(filename);
        //llvm::raw_os_ostream Out(ofstr);
        llvm::WriteBitcodeToFile(module, Out);
        Out.flush();
        return true;
    }
 
#ifdef KLEE_SEMU_GENMU_OBJECTFILE    
    static bool writeObj (llvm::Module * module, const std::string filename)
    {
        auto TargetTriple = llvm::sys::getDefaultTargetTriple();
        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmParsers();
        llvm::InitializeAllAsmPrinters();
        
        std::string Error;
        auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

        // Print an error and exit if we couldn't find the requested target.
        // This generally occurs if we've forgotten to initialise the
        // TargetRegistry or we have a bogus target triple.
        if (!Target) {
          llvm::errs() << Error;
          return 1;
        }

        auto CPU = std::string("generic");
        auto Features = std::string("");

        llvm::TargetOptions opt;
        auto RM = llvm::Optional<llvm::Reloc::Model>();
        auto TargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, *RM);
        
        module->setDataLayout(TargetMachine->createDataLayout());
        module->setTargetTriple(TargetTriple);

#if (LLVM_VERSION_MAJOR <= 3) && (LLVM_VERSION_MINOR < 5)
        std::string estr("");
        llvm::raw_fd_ostream Out(filename.c_str(), estr, llvm::sys::fs::F_None);
        if (estr.length() > 0) 
        {
            llvm::errs() << "Could not open file: " << estr;
            return false;
        }
        llvm::PassManager pass;
#else
        std::error_code EC;
        llvm::raw_fd_ostream Out(filename, EC, llvm::sys::fs::F_None);
        if (EC) 
        {
            llvm::errs() << "Could not open file: " << EC.message();
            return false;
        }
        llvm::legacy::PassManager pass;
#endif
        
        auto FileType = TargetMachine->CGFT_ObjectFile;

        if (TargetMachine->addPassesToEmitFile(pass, Out, FileType)) {
          llvm::errs() << "TargetMachine can't emit a file of this type";
          return 1;
        }

        pass.run(*module);
        Out.flush();

        return true;
    }
#endif  //#ifdef KLEE_SEMU_GENMU_OBJECTFILE  
};