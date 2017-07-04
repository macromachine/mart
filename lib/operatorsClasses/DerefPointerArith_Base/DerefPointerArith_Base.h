#ifndef __KLEE_SEMU_GENMU_operatorClasses__DerefPointerArith_Base__
#define __KLEE_SEMU_GENMU_operatorClasses__DerefPointerArith_Base__

/**
 * -==== DerefPointerArith_Base.h
 *
 *                MuLL Multi-Language LLVM Mutation Framework
 *
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE.TXT for details. 
 *  
 * \brief     Generic abstract base class for all mutation operator that match and replace a combination of arithmetic increment/decrement operation and pointer dereference.
 * \details   This abstract class define is extended from the Generic base class. 
 */
 
#include "GenericMuOpBase.h"

class DerefPointerArith_Base: public GenericMuOpBase
{
  protected:
    /**
     * \brief This method is to be implemented by each deref-arith matching-mutating operator
     * @return the constant key in user maps for the arithmetic numeric/pointer operation that is composing this operation together with deref
     */
    inline virtual enum ExpElemKeys getCorrespondingAritPtrOp() = 0;
    
    /**
     * \brief Say wheter the operation dereference first (ex: (*p)--, *(p)+9, ...)
     * @return true if the dereference first, else false.
     */
    inline virtual bool dereferenceFirst() = 0;
    
    inline virtual bool checkPtrContainedType(llvm::Value *pval)
    {
        assert (false && "Must be implemented by any operation whit NOT deref first");
    }
    
    inline virtual bool isCommutative()
    {
        return false;
    }
    /**
     *  \brief
     * @return the single use of the IR at position pos, and its position in 'ret_pos'. if do not have single use, return nullptr. 
     */
    inline llvm::Value * getSingleUsePos (MatchStmtIR const &toMatch, unsigned pos, unsigned &ret_pos)
    {
        auto * tmp = toMatch.getIRAt(pos);
        if (tmp->hasOneUse())
        {
            ret_pos = toMatch.depPosofPos(tmp->user_back(), pos, false);
            return tmp->user_back();
        }
        ret_pos = -1;  //max unsigned
        return nullptr;
    }
    
  public:
    bool matchIRs (MatchStmtIR const &toMatch, llvmMutationOp const &mutationOp, unsigned pos, MatchUseful &MU, ModuleUserInfos const &MI) 
    {
        llvm::Value *val = toMatch.getIRAt(pos);
        if (MI.getUserMaps()->getMatcherObject(getCorrespondingAritPtrOp())->matchIRs(toMatch, mutationOp, pos, MU, MI))
        {
            assert (MU.next() == MU.end() && "Must have exactely one element here (deref first)");
            if (dereferenceFirst())
            {
                llvm::SmallVector<std::tuple<unsigned/*HLoprd ind*/, unsigned/*gep pos*/, unsigned/*load pos*/>, 2> hloprd_reset_data;
                unsigned noprd = MU.getNumberOfHLOperands();
                assert ((noprd >= 1 && noprd <= 2) && "must be binop or incdec");
                for (unsigned hloprd_id = 0, hloprd_end=(isCommutative()?1:noprd); hloprd_id < hloprd_end ; ++hloprd_id)
                {
                    if (auto *load = llvm::dyn_cast<llvm::LoadInst>(MU.getHLOperandSource(hloprd_id, toMatch)))
                    {
                        auto *gep = llvm::dyn_cast<llvm::GetElementPtrInst>(load->getPointerOperand());
                        int indx;
                        if (gep && checkIsPointerIndexingAndGet(gep, indx) && indx == gep->getNumIndices()-1/*no other indices after array index*/)
                        {
                            //Modify MU, setting load pointer operand as HL perand instead of arith operands
                            hloprd_reset_data.emplace_back(hloprd_id, toMatch.depPosofPos(gep, pos, true), toMatch.depPosofPos(load, pos, true));
                        }
                    }
                }
                if (hloprd_reset_data.empty())
                {
                    MU.clearAll();
                }
                else
                {
                    MatchUseful *ptr_mu = &MU;
                    for (auto i=hloprd_reset_data.size()-1; i>0; --i)   //if exactely 1 element, no need to getNew, since we can directly use the ptr_mu returned by arith matcher
                        ptr_mu = ptr_mu->getNew_CopyData();
                    unsigned i=0;
                    for (ptr_mu = MU.first(); ptr_mu != MU.end(); ptr_mu = ptr_mu->next(), ++i)
                    {
                        if (std::get<0>(hloprd_reset_data[i]) > 0)  //actually ==1
                        {
                            ptr_mu->resetHLOprdSourceAt(std::get<0>(hloprd_reset_data[i]), ptr_mu->getHLOperandSourcePos(0));  // non pointeris oprd 0, make it become oprd 1
                        }
                        ptr_mu->resetHLOprdSourceAt(0, std::get<1>(hloprd_reset_data[i]));
                        ptr_mu->appendRelevantIRPos(std::get<2>(hloprd_reset_data[i]));
                    }
                }
            }
            else
            {
                unsigned loadPos;
                if (checkPtrContainedType(toMatch.getIRAt(MU.getHLReturningIRPos())) 
                        && llvm::dyn_cast_or_null<llvm::LoadInst>(getSingleUsePos(toMatch, MU.getHLReturningIRPos()/*Must have returningIR*/, loadPos)))
                {
                    //Modify MU, setting returning IR pos to loadPos
                    MU.setHLReturningIRPos(loadPos);
                    MU.appendRelevantIRPos(loadPos);
                }
                else
                {
                    MU.clearAll();
                }
            }
        }
        else
        {
            MU.clearAll();
        }
        
        return (MU.first() != MU.end());
    }
    
    void prepareCloneIRs (MatchStmtIR const &toMatch, unsigned pos,  MatchUseful const &MU, llvmMutationOp::MutantReplacors const &repl, DoReplaceUseful &DRU, ModuleUserInfos const &MI)
    {
        DRU.toMatchMutant.setToCloneStmtIROf(toMatch, MI);
        llvm::Value * ptroprd = nullptr, *valoprd = nullptr;
        // the pointer oprd cannot be replaced by 0 since we need to dereference
        if (repl.getOprdIndexList().size() == 2)    //size is 2
        {
            ptroprd = MU.getHLOperandSource(repl.getOprdIndexList()[0], DRU.toMatchMutant);      //pointer
            valoprd = createIfConst ((*(llvm::dyn_cast<llvm::GetElementPtrInst>(MU.getHLOperandSource(0, DRU.toMatchMutant)))->idx_begin())->getType(), repl.getOprdIndexList()[1]);
            if (!valoprd)
            {
                assert (MU.getNumberOfHLOperands() == 2 && "Problem here, must be hard coded constant int here");
                valoprd = MU.getHLOperandSource(repl.getOprdIndexList()[1], DRU.toMatchMutant);
            }
        }
        else    //size is 1
        {
            if ( ptroprd = createIfConst ((*(llvm::dyn_cast<llvm::GetElementPtrInst>(MU.getHLOperandSource(0, DRU.toMatchMutant)))->idx_begin())->getType(), repl.getOprdIndexList()[0]))
            { //The replacor should be CONST_VALUE_OF
                llvm::IRBuilder<> builder (MI.getContext());
                ptroprd = builder.CreateIntToPtr(ptroprd, (DRU.toMatchMutant.getIRAt(MU.getHLReturningIRPos()))->getType());
                if (! llvm::isa<llvm::Constant>(ptroprd))
                    DRU.toMatchMutant.insertIRAt(*std::max_element(MU.getRelevantIRPos().begin(), MU.getRelevantIRPos().end()) + 1, ptroprd);    //insert right after the instruction to remove
            }
            else // pointer
            {
                ptroprd = MU.getHLOperandSource(repl.getOprdIndexList()[0], DRU.toMatchMutant);      //load. repl.getOprdIndexList()[0] is equal to 0 here
            }
        }
        
        for (auto p : MU.getRelevantIRPos())
            DRU.posOfIRtoRemove.push_back(p);
        DRU.appendHLOprds(ptroprd);
        DRU.appendHLOprds(valoprd);
        DRU.setOrigRelevantIRPos(MU.getRelevantIRPos());
        DRU.setHLReturningIRPos(MU.getHLReturningIRPos());
    }
};

#endif //__KLEE_SEMU_GENMU_operatorClasses__DerefPointerArith_Base__