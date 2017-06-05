#! /usr/bin/python

##
# NOTE: each operation is kind of symetric, meaning that when it has operands, all operands are of same type (arith or pointer)
##~~

import os, sys
import re

class GlobalDefs:
    #get the oprd type in oprdTake including the most others and and which is in oprdCheck
    def getMaxOprdsType (self, oprdTake, oprdCheck):
        if self.ARITHEXPR in oprdTake:
            tmp = [self.ARITHEXPR]+list(oprdTake - {self.ARITHEXPR})
        elif self.POINTEREXPR in oprdTake:
            tmp = [self.POINTEREXPR]+list(oprdTake - {self.POINTEREXPR})
        else:
            tmp =  list(oprdTake)
        if oprdCheck is None:
            return tmp[0]
        else:
            for xx in tmp:
                if xx in oprdCheck:
                    return xx
            return None
        
    def __init__(self):
        myNumOfReplFuncs = 1
        assert myNumOfReplFuncs >= 1, "myNumOfReplFuncs must be higher than 0"
    
        self.ZERO = "0"
        self.SCALAR_CONST = {"-1", "1"} #COnstant for things likeincreasing, decreasing constant or expression
        self.COUNTERS = {"1"}    #counter for thins like SHUFFLE
        self.SCALAR_STRING = {"dirname", "basename"}    #string for things like callee replacement
      # Match only
        # Values
        self.ARITHEXPR = "@"
        self.ARITHVAR = "V"
        self.ARITHCONST = "C"
        self.arithVAL = {self.ARITHEXPR, self.ARITHVAR, self.ARITHCONST}
        self.POINTERVAR = "P"
        self.POINTEREXPR = "A"
        self.pointerVAL = {self.POINTERVAR, self.POINTEREXPR}
        # Structure
        self.MATCH_ONLY_DEL = ("STMT", "RETURN_BREAK_CONTINUE")
        self.CALL = ("CALL",)
        self.SWITCH = ("SWITCH",)
        matchOnlys = self.arithVAL | self.pointerVAL | set(self.MATCH_ONLY_DEL) | set(self.CALL) | set (self.SWITCH) #local var just for internal check
        
      # Replace Only
        self.DELSTMT = ("DELSTMT",)
        self.KEEPOPERAND = ("OPERAND",)
        self.CONSTVAL = ("CONSTVAL",)
        self.NEWCALLEE = ("NEWCALLEE",)
        self.SHUFFLE_ARGS_CASES = ("SHUFFLE",)
        
        self.ABS_UNARY = ("ABS",)
        replaceOnlys = set(self.ABS_UNARY + self.DELSTMT + self.KEEPOPERAND + self.CONSTVAL + self.NEWCALLEE + self.SHUFFLE_ARGS_CASES) #local var just for internal check
        
      # Both Match and Replace
        # Arithmetic
        self.AOR = ("ADD", "SUB", "MUL", "DIV", "MOD")
        self.BIT = ("BITAND", "BITOR", "BITXOR", "BITSHL", "BITSHR")
        self.ROR = ("EQ", "NEQ", "LT", "GT", "LE", "GE")
        self.ASSIGN = ("ASSIGN",)
        self.UNARY = ("BITNOT", "NEG")
        self.INCDEC = ("LEFTINC", "LEFTDEC", "RIGHTINC", "RIGHTDEC")
        # Pointer
        self.P_AOR = ("PADD", "PSUB")
        self.P_ROR = ("PEQ", "PNEQ", "PLT", "PGT", "PLE", "PGE")
        self.P_INCDEC = ("PLEFTINC", "PLEFTDEC", "PRIGHTINC", "PRIGHTDEC")
        # Logical 
        self.LOR = ("AND", "OR")
        
      #####
        self.commutativeBinop = {"ADD", "MUL", "BITAND", "BITOR", "BITXOR", "EQ", "NEQ", "PEQ", "PNEQ"} | set(self.LOR) | set(self.P_AOR)
        # Key: operation, Value: list of operators 'types'
        self.FORMATS = {}
        self.FORMATS.update({opName: [self.arithVAL, self.arithVAL] for opName in self.AOR + self.BIT + self.ROR + self.LOR})
        self.FORMATS.update({opName: [{self.ARITHVAR}, self.arithVAL] for opName in self.ASSIGN})
        self.FORMATS.update({opName: [self.pointerVAL, self.arithVAL] for opName in self.P_AOR + self.P_ROR})
        self.FORMATS.update({opName: [self.arithVAL] for opName in self.UNARY + self.ABS_UNARY})
        self.FORMATS.update({opName: [{self.ARITHVAR}] for opName in self.INCDEC})
        self.FORMATS.update({opName: [{self.POINTERVAR}] for opName in self.P_INCDEC})

        self.FORMATS.update({opName: [] for opName in tuple(self.arithVAL | self.pointerVAL) + self.MATCH_ONLY_DEL + self.CALL + self.SWITCH})   #Match only's
        self.FORMATS.update({opName: [] for opName in self.DELSTMT})                                                                             #Replace only's

        self.FORMATS.update({opName: [self.arithVAL|self.pointerVAL] for opName in self.KEEPOPERAND})
        self.FORMATS.update({opName: [self.SCALAR_CONST|{self.ZERO}] for opName in self.CONSTVAL})
        self.FORMATS.update({opName: [self.SCALAR_STRING]*(1+myNumOfReplFuncs) for opName in self.NEWCALLEE})           # 1+... because of the matched function (param number 1)
        self.FORMATS.update({opName: [self.COUNTERS] for opName in self.SHUFFLE_ARGS_CASES})
         
        arithOps = self.AOR + self.BIT + self.ROR + self.ASSIGN + self.UNARY + self.P_INCDEC  # + self.LOR        #as for now, LOR can only be replaced(replacing) LOR TODO TODO: add here when supported
        arthExtraRepl = self.DELSTMT + self.KEEPOPERAND + self.CONSTVAL + self.ABS_UNARY
        pointerOps = self.P_AOR + self.P_INCDEC
        pointerExtraRepl = self.DELSTMT + self.KEEPOPERAND

        # Who can replace who: key is matcher, value is replcor
        # Key: operation, Value: set or possible replacors
        self.RULES = {}
        self.RULES.update({opName: set(arithOps + arthExtraRepl) - {opName} for opName in arithOps})
        self.RULES.update({opName: set(pointerOps + pointerExtraRepl) - {opName} for opName in pointerOps})
        self.RULES.update({opName: set(self.P_ROR + pointerExtraRepl) - {opName} for opName in self.P_ROR}) # pointer relational only with pointer relational or del or keepoprd
        self.RULES.update({opName: set(self.LOR + self.DELSTMT + self.KEEPOPERAND + self.CONSTVAL) - {opName} for opName in self.LOR})

        self.RULES.update({self.ARITHEXPR: set(arithOps + arthExtraRepl) - set(self.KEEPOPERAND) - set(self.INCDEC)})
        self.RULES.update({self.ARITHVAR: set(arithOps + arthExtraRepl) - set(self.KEEPOPERAND)})
        self.RULES.update({self.ARITHCONST: set(arithOps + arthExtraRepl) - set(self.KEEPOPERAND) - set(self.INCDEC)})

        self.RULES.update({self.POINTEREXPR: set(pointerOps + self.DELSTMT) - set(self.P_INCDEC)})
        self.RULES.update({self.POINTERVAR: set(pointerOps + self.DELSTMT)})

        self.RULES.update({opName: set(self.DELSTMT) for opName in self.MATCH_ONLY_DEL})
        self.RULES.update({opName: set(self.DELSTMT + self.NEWCALLEE + self.SHUFFLE_ARGS_CASES) for opName in self.CALL})
        self.RULES.update({opName: set(self.DELSTMT + self.SHUFFLE_ARGS_CASES) for opName in self.SWITCH})
         
        for tmpM in self.RULES:
            assert tmpM not in replaceOnlys, "replaceOnlys considered as matcher in RULES"
            for tmpR in self.RULES[tmpM]:
                assert tmpR not in matchOnlys, "matchOnlys considered as replacor in RULES"
         
#~ GlobalDefs

#Process Match Replace, add a new replacement for key in the map(in the list of repls)
def processMR (matchOp, matchOprds, repOp, replOprds, tmpStrsMap):
    # matchOp(matchOprds[0], matchOprds[1],..) or matchOp is key in tmpStrsMap
    # ['NAME, repOp(replOprds[0],..)', ...] is the value in tmpStrsMap
    if not ( matchOp in globalDefs.RULES and repOp in globalDefs.RULES[matchOp]):
        print "match op:", matchOp, "replace op:", repOp
        assert false, "wrong matchOp and replaceOp"
    key = matchOp
    if len(matchOprds) > 0:
        key += "(" + ", ".join(matchOprds) + ")"
    if key not in tmpStrsMap:
        tmpStrsMap[key] = []
    valuei = repOp
    if len(replOprds) > 0:
        valuei += "(" + ", ".join(replOprds) + ")"
    # construct name with key and value, replacing parenthesis and comma by 'nameoprdSep' and separate the two by a 'matchreplSep'
    matchreplSep = "!"
    nameoprdSep = "$"
    name = matchreplSep.join([re.sub('[,()]', nameoprdSep, key), re.sub('[,()]', nameoprdSep, valuei)])
    name = name.replace(' ','')
    
    valuei = ', '.join([name, valuei])
    
    tmpStrsMap[key].append(valuei)
#~ def processMR()

def getAllPossibleMConf():
    outStr = "## Automatically generated mutant conf for MuLL ##\n\n"
    for op in globalDefs.RULES:
        tmpStrsMap = {}
        # If it is a matcher that do not have operands, process here
        if len(globalDefs.FORMATS[op]) == 0:
            if op == globalDefs.ARITHEXPR:  #any arith expr @ --> ABS(@)
                for rep in globalDefs.RULES[op]:
                    if rep in globalDefs.ABS_UNARY:
                        processMR (op, (), rep, (op,), tmpStrsMap)
            elif op == globalDefs.ARITHCONST:  #const c --> c+1, c-1, 0
                add = "ADD"
                constval = "CONSTVAL"
                assert (add in globalDefs.RULES[op]), "ADD not in replacors for C"
                assert (constval in globalDefs.RULES[op]), "CONSTVAL not in replacors for C"
                for sc in globalDefs.SCALAR_CONST:
                    processMR (op, (), add, (op, sc), tmpStrsMap)
                processMR (op, (), constval, (globalDefs.ZERO,), tmpStrsMap)
            elif op == globalDefs.ARITHVAR:  #var v --> ++v, --v,..
                for rep in globalDefs.RULES[op]:
                    if rep in globalDefs.INCDEC:
                        processMR (op, (), rep, (op,), tmpStrsMap)
            elif op == globalDefs.POINTERVAR:  #pointer p --> ++p, --p,..
                continue   #TODO TODO remove this when fixed for P
                for rep in globalDefs.RULES[op]:
                    if rep in globalDefs.P_INCDEC:
                        processMR (op, (), rep, (op,), tmpStrsMap)
            elif op == globalDefs.POINTEREXPR:
                continue
            elif op in globalDefs.MATCH_ONLY_DEL: #both match and replacer have no oprds
                for rep in globalDefs.RULES[op]:
                    processMR (op, (), rep, (), tmpStrsMap)
            elif op in globalDefs.CALL:
                continue   #TODO TODO remove this when fixed for shuffle
                for rep in globalDefs.RULES[op]:
                    if rep in globalDefs.SHUFFLE_ARGS_CASES:
                        for ct in globalDefs.COUNTERS:
                            processMR (op, (), rep, (ct,), tmpStrsMap)
                    elif rep in globalDefs.NEWCALLEE:
                        assert len(globalDefs.FORMATS[rep]) == 2, "FIXME: fix bellow to support more than one function replaced"
                        for ncm in globalDefs.SCALAR_STRING:
                            for ncr in set(globalDefs.SCALAR_STRING) - {ncm}:
                                processMR (op, (), rep, (ncm, ncr), tmpStrsMap)
            elif op in globalDefs.SWITCH:   #only shuffle is needed here
                continue   #TODO TODO remove this when fixed for shuffle
                for rep in globalDefs.RULES[op]:
                    if rep in globalDefs.SHUFFLE_ARGS_CASES:
                        for ct in globalDefs.COUNTERS:
                            processMR (op, (), rep, (ct,), tmpStrsMap)
            else:
                print "match op:", op
                assert False, "error: unreachable -- op has no oprd"
        else:  #the matcher has operands
            for repl in globalDefs.RULES[op]:
                # if op has no oprd
                if len(globalDefs.FORMATS[repl]) == 0:
                    matchOprdsL = []
                    for ind,moprd in enumerate(globalDefs.FORMATS[op]):
                        matchOprdsL.append(globalDefs.getMaxOprdsType (moprd, None)+str(ind+1))
                    processMR (op, tuple(matchOprdsL), repl, (), tmpStrsMap)
                # if op has one oprd
                elif len(globalDefs.FORMATS[repl]) == 1:
                    for moprdind in range(len(globalDefs.FORMATS[op])):
                        soprd = globalDefs.getMaxOprdsType (globalDefs.FORMATS[op][moprdind], globalDefs.FORMATS[repl][0])
                        if soprd is None:   # the operand types didn't match
                            continue
                        matchOprdsL = []
                        for ind,moprd in enumerate(globalDefs.FORMATS[op]):
                            if ind == moprdind:
                                soprd += str(ind+1)
                                matchOprdsL.append(soprd)
                            else:
                                matchOprdsL.append(globalDefs.getMaxOprdsType (moprd, None)+str(ind+1))
                        processMR (op, tuple(matchOprdsL), repl, (soprd,), tmpStrsMap)
                # if op has two oprds
                elif len(globalDefs.FORMATS[repl]) == 2:
                    for rh_moprdind in range(len(globalDefs.FORMATS[op])):
                        for lh_moprdind in range(rh_moprdind):
                            for swapctrl in [0, 1]:
                                if swapctrl == 1 and repl in globalDefs.commutativeBinop:  #no need to swap operand for commutative replacers
                                    continue
                                tmp_lh_moprdind = lh_moprdind if swapctrl == 0 else rh_moprdind
                                tmp_rh_moprdind = rh_moprdind if swapctrl == 0 else lh_moprdind
                                    
                                lh_soprd = globalDefs.getMaxOprdsType (globalDefs.FORMATS[op][tmp_lh_moprdind], globalDefs.FORMATS[repl][0])
                                rh_soprd = globalDefs.getMaxOprdsType (globalDefs.FORMATS[op][tmp_rh_moprdind], globalDefs.FORMATS[repl][1])
                                if lh_soprd is None or rh_soprd is None:   # the operands types didn't match
                                    continue
                                matchOprdsL = []
                                for ind,moprd in enumerate(globalDefs.FORMATS[op]):
                                    if ind == tmp_lh_moprdind:
                                        lh_soprd += str(ind+1)
                                        matchOprdsL.append(lh_soprd)
                                    elif ind == tmp_rh_moprdind:
                                        rh_soprd += str(ind+1)
                                        matchOprdsL.append(rh_soprd)
                                    else:
                                        matchOprdsL.append(globalDefs.getMaxOprdsType (moprd, None)+str(ind+1))
                                processMR (op, tuple(matchOprdsL), repl, (lh_soprd, rh_soprd), tmpStrsMap)
                else:
                    assert False, "invalid number of operands for replacer"
        for line in tmpStrsMap:
            outStr += line + " --> " + "; ".join(tmpStrsMap[line]) + ";\n"
    return outStr
#~ def getMConf()

def main(args):
    assert len(args) == 2, "expected 1 argument: output file"
    outfile = args[1]
    
    mconfString = getAllPossibleMConf()
    with open(outfile, "w") as fp:
        fp.write(mconfString)
    print "\n@ mutation config successfully written in file", outfile, "\n"
#~ def main()
   
globalDefs = GlobalDefs()
if __name__ == "__main__":
    main(sys.argv)
    