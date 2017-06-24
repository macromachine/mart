#ifndef __KLEE_SEMU_GENMU_operatorClasses__ShuffleArgs__
#define __KLEE_SEMU_GENMU_operatorClasses__ShuffleArgs__

/**
 * -==== ShuffleArgs.h
 *
 *                MuLL Multi-Language LLVM Mutation Framework
 *
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE.TXT for details. 
 *  
 * \brief    Class for shuffling function call arguments of same type. Actually inexistant. only the class FunctionCall can implements and use such replacer
 */
 
#include "../ReplaceOnly_Base.h"

class ShuffleArgs: public ReplaceOnly_Base
{
    /**
     * \bref Inplements virtual from @see GenericMuOpBase
     */
    llvm::Value * createReplacement (llvm::Value * oprd1_addrOprd, llvm::Value * oprd2_intValOprd, std::vector<llvm::Value *> &replacement, ModuleUserInfos const &MI)
    {
        llvm::errs() << "Error: the class ShuffleArgs is unusable. only FunctionCall can implements and use such replacer.\n";
        assert (false);
    }
};

#endif //__KLEE_SEMU_GENMU_operatorClasses__ShuffleArgs__