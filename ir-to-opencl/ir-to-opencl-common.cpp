#include <iostream>
#include <sstream>

#include "llvm/IR/AssemblyAnnotationWriter.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "ir-to-opencl-common.h"

using namespace std;
using namespace llvm;

std::string dumpType(Type *type);

extern bool single_precision;
// extern llvm::LLVMContext TheContext;

GlobalVariable *addGlobalVariable(Module *M, string name, string value) {
    // string name = ".mystr1";
    // string desiredString = value;
    int N = value.size() + 1;
    LLVMContext &context = M->getContext();
    ArrayType *strtype = ArrayType::get(IntegerType::get(context, 8), N);
    M->getOrInsertGlobal(StringRef(name), strtype);
    // ConstantDataSequential *charConstSeq = cast<ConstantDataSequential>(charConst);

    ConstantDataArray *constchararray = cast<ConstantDataArray>(ConstantDataArray::get(context, ArrayRef<uint8_t>((uint8_t *)value.c_str(), N)));
    GlobalVariable *str = M->getNamedGlobal(StringRef(name));
    str->setInitializer(constchararray);
    return str;
}

Instruction *addStringInstr(Module *M, string name, string value) {
    // Module *M = prev->getParent();
    GlobalVariable *var = addGlobalVariable(M, name, value);

    int N = value.size() + 1;
    LLVMContext &context = M->getContext();
    ArrayType *arrayType = ArrayType::get(IntegerType::get(context, 8), N);
    Value * indices[2];
    indices[0] = ConstantInt::getSigned(IntegerType::get(context, 32), 0);
    indices[1] = ConstantInt::getSigned(IntegerType::get(context, 32), 0);
    GetElementPtrInst *elem = GetElementPtrInst::CreateInBounds(arrayType, var, ArrayRef<Value *>(indices, 2));
    // elem->insertAfter(prev);
    return elem;
}

std::string dumpFunctionType(FunctionType *fn) {
    cout << "function" << endl;
    // cout << "name " << string(fn->getName()) << endl;
    std::string params_str = "";
    int i = 0;
    for(auto it=fn->param_begin(); it != fn->param_end(); it++) {
        Type * paramType = *it;
        if(i > 0) {
            params_str += ", ";
        }
        params_str += dumpType(paramType);
        i++;
    }
    cout << "params_str " << params_str << endl;
    return params_str;
}

std::string dumpPointerType(PointerType *ptr) {
    string gencode = "";
    Type *elementType = ptr->getPointerElementType();
    string elementTypeString = dumpType(elementType);
    int addressspace = ptr->getAddressSpace();
    if(addressspace == 1) {
        gencode += "global ";
    }
    gencode += elementTypeString + "*";
    return gencode;
}

std::string dumpIntegerType(IntegerType *type) {
    switch(type->getPrimitiveSizeInBits()) {
        case 32:
            return "int";
        case 64:
            return "long";
        case 8:
            return "char";
        case 1:
            return "bool";
        default:
            cout << "integer size " << type->getPrimitiveSizeInBits() << endl;
            throw runtime_error("unrecognized size");
    }
}

std::string dumpStructType(StructType *type) {
    if(type->hasName()) {
        string name = type->getName();
        if(name == "struct.float4") {
            return "float4";
        } else {
            cout << "struct name: " << name << endl;
            throw runtime_error("not implemented: struct name " + name);
        }
    } else {
        throw runtime_error("not implemented: anonymous struct types");
    }
}

std::string dumpType(Type *type) {
    Type::TypeID typeID = type->getTypeID();
    switch(typeID) {
        case Type::VoidTyID:
            return "void";
        case Type::FloatTyID:
            return "float";
        // case Type::UnionTyID:
        //     throw runtime_error("not implemented: union type");
        case Type::StructTyID:
            return dumpStructType((StructType *)type);
        case Type::VectorTyID:
            throw runtime_error("not implemented: vector type");
        case Type::ArrayTyID:
            throw runtime_error("not implemented: array type");
        case Type::DoubleTyID:
            if(single_precision) {
                return "float";
            } else {
                return "double";
            }
        case Type::FunctionTyID:
            return dumpFunctionType(dyn_cast<FunctionType>(type));
        case Type::PointerTyID:
            // cout << "pointer type" << endl;
            return dumpPointerType((PointerType *)type);
        case Type::IntegerTyID:
            return dumpIntegerType((IntegerType *)type);
        default:
            cout << "type id " << typeID << endl;
            throw runtime_error("unrecognized type");
    }
}
