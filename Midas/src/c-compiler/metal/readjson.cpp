#include <iostream>

#include "readjson.h"
#include "metal/instructions.h"
#include "metal/ast.h"
#include "metalcache.h"

// for convenience
using json = nlohmann::json;

Reference* readReference(MetalCache* cache, const json& reference);
Ownership readOwnership(MetalCache* cache, const json& ownership);
Location readLocation(MetalCache* cache, const json& location);
Mutability readMutability(const json& mutability);

//template<typename T>
//concept ReturnsVec = requires(T a) {
//  { std::hash<T>{}(a) } -> std::convertible_to<std::size_t>;
//};
template<
    typename F,
    typename T = decltype((*(const F*)nullptr)(nullptr, *(const json*)nullptr))>
std::vector<T> readArray(MetalCache* cache, const json& j, const F& f) {
  assert(j.is_array());
  auto vec = std::vector<T>{};
  for (const auto& element : j) {
    vec.push_back(f(cache, element));
  }
  return vec;
}
// F should return pair<key, value>
template<typename K, typename V, typename F>
std::unordered_map<K, V> readArrayIntoMap(MetalCache* cache, const json& j, const F& f) {
  assert(j.is_array());
  auto map = std::unordered_map<K, V>{};
  map.reserve(j.size());
  for (const auto& element : j) {
    std::pair<K, V> p = f(cache, element);
//    assert(map.find(p.first) == map.end());
    map.emplace(move(p.first), move(p.second));
  }
  return map;
}

Name* readName(MetalCache* cache, const json& name) {
  assert(name.is_string());
  auto nameStr = name.get<std::string>();

  return makeIfNotPresent(
      &cache->names,
      nameStr,
      [&](){ return new Name(nameStr); });
}

StructReferend* readStructReferend(MetalCache* cache, const json& referend) {
  assert(referend[""] == "StructId");

  auto structName = readName(cache, referend["name"]);

  auto result =
      makeIfNotPresent(
          &cache->structReferends,
          structName,
          [&]() { return new StructReferend(structName); });

  if (structName->name == "Tup") {
    cache->emptyTupleStructReferend = result;
  }

  return result;
}

InterfaceReferend* readInterfaceReferend(MetalCache* cache, const json& referend) {
  assert(referend[""] == "InterfaceId");

  auto interfaceName = readName(cache, referend["name"]);

  return makeIfNotPresent(
      &cache->interfaceReferends,
      interfaceName,
      [&](){ return new InterfaceReferend(interfaceName); });
}

RawArrayT* readRawArray(MetalCache* cache, const json& rawArray) {
  assert(rawArray[""] == "Array");

  auto mutability = readMutability(rawArray["mutability"]);
  auto elementType = readReference(cache, rawArray["elementType"]);

  return makeIfNotPresent(
      &cache->rawArrays[elementType],
      mutability,
      [&](){ return new RawArrayT(mutability, elementType); });
}

UnknownSizeArrayT* readUnknownSizeArray(MetalCache* cache, const json& referend) {
  auto name = readName(cache, referend["name"]);
  auto rawArray = readRawArray(cache, referend["array"]);

  return makeIfNotPresent(
      &cache->unknownSizeArrays,
      name,
      [&](){ return new UnknownSizeArrayT(name, rawArray); });
}

KnownSizeArrayT* readKnownSizeArray(MetalCache* cache, const json& referend) {
  auto name = readName(cache, referend["name"]);
  auto rawArray = readRawArray(cache, referend["array"]);
  auto size = referend["size"].get<int>();

  return makeIfNotPresent(
      &cache->knownSizeArrays,
      name,
      [&](){ return new KnownSizeArrayT(name, size, rawArray); });
}

Referend* readReferend(MetalCache* cache, const json& referend) {
  assert(referend.is_object());
  if (referend[""] == "Int") {
    return cache->innt;
  } else if (referend[""] == "Bool") {
    return cache->boool;
  } else if (referend[""] == "Str") {
    return cache->str;
  } else if (referend[""] == "Void") {
    return cache->vooid;
  } else if (referend[""] == "StructId") {
    return readStructReferend(cache, referend);
  } else if (referend[""] == "Never") {
    return cache->never;
  } else if (referend[""] == "UnknownSizeArray") {
    return readUnknownSizeArray(cache, referend);
  } else if (referend[""] == "KnownSizeArray") {
    return readKnownSizeArray(cache, referend);
  } else if (referend[""] == "InterfaceId") {
    return readInterfaceReferend(cache, referend);
  } else {
    std::cerr << "Unrecognized referend: " << referend[""] << std::endl;
    assert(false);
  }
}

Reference* readReference(MetalCache* cache, const json& reference) {
  assert(reference.is_object());
  assert(reference[""] == "Ref");

  auto ownership = readOwnership(cache, reference["ownership"]);
  auto location = readLocation(cache, reference["location"]);
  auto referend = readReferend(cache, reference["referend"]);
//  std::string debugStr = reference["debugStr"];

  return makeIfNotPresent(
      &cache->references[referend][ownership],
      location,
      [&](){ return new Reference(ownership, location, referend); });
}

Mutability readMutability(const json& mutability) {
  assert(mutability.is_object());
  if (mutability[""].get<std::string>() == "Mutable") {
    return Mutability::MUTABLE;
  } else if (mutability[""].get<std::string>() == "Immutable") {
    return Mutability::IMMUTABLE;
  } else {
    assert(false);
  }
}

Variability readVariability(const json& variability) {
  assert(variability.is_object());
  if (variability[""].get<std::string>() == "Varying") {
    return Variability::VARYING;
  } else if (variability[""].get<std::string>() == "Final") {
    return Variability::FINAL;
  } else {
    assert(false);
  }
}

Ownership readOwnership(MetalCache* cache, const json& ownership) {
  assert(ownership.is_object());
//  std::cout << ownership.type() << std::endl;
  if (ownership[""].get<std::string>() == "Own") {
    return Ownership::OWN;
  } else if (ownership[""].get<std::string>() == "Borrow") {
    return Ownership::BORROW;
  } else if (ownership[""].get<std::string>() == "Weak") {
    return Ownership::WEAK;
  } else if (ownership[""].get<std::string>() == "Share") {
    return Ownership::SHARE;
  } else {
    assert(false);
  }
}

Location readLocation(MetalCache* cache, const json& location) {
  assert(location.is_object());
//  std::cout << location.type() << std::endl;
  if (location[""].get<std::string>() == "Inline") {
    return Location::INLINE;
  } else if (location[""].get<std::string>() == "Yonder") {
    return Location::YONDER;
  } else {
    assert(false);
  }
}

Prototype* readPrototype(MetalCache* cache, const json& prototype) {
  assert(prototype.is_object());
  assert(prototype[""] == "Prototype");

  auto name = readName(cache, prototype["name"]);
  auto params = readArray(cache, prototype["params"], readReference);
  auto retuurn = readReference(cache, prototype["return"]);

  return makeIfNotPresent(
      &cache->prototypes[name][retuurn],
      params,
      [&](){ return new Prototype(name, params, retuurn); });
}

VariableId* readVariableId(MetalCache* cache, const json& variable) {
  assert(variable.is_object());
  assert(variable[""] == "VariableId");

  int number = variable["number"];
  std::string maybeName;
  if (variable["name"][""] == "Some") {
    maybeName = variable["name"]["value"];
  }

  return makeIfNotPresent(
      &cache->variableIds[number],
      maybeName,
      [&](){ return new VariableId(number, maybeName); });
}

Local* readLocal(MetalCache* cache, const json& local) {
  assert(local.is_object());
  assert(local[""] == "Local");
  auto varId = readVariableId(cache, local["id"]);
  auto ref = readReference(cache, local["type"]);

  return makeIfNotPresent(
      &cache->locals[varId],
      ref,
      [&](){ return new Local(varId, ref); });
}

Expression* readExpression(MetalCache* cache, const json& expression) {
  assert(expression.is_object());
  std::string type = expression[""];
  if (type == "ConstantI64") {
    return new ConstantI64(
        expression["value"]);
  } else if (type == "ConstantBool") {
    return new ConstantBool(
        expression["value"]);
  } else if (type == "Return") {
    return new Return(
        readExpression(cache, expression["sourceExpr"]),
        readReference(cache, expression["sourceType"]));
  } else if (type == "Stackify") {
    return new Stackify(
        readExpression(cache, expression["sourceExpr"]),
        readLocal(cache, expression["local"]),
        "");
  } else if (type == "LocalStore") {
    return new LocalStore(
        readLocal(cache, expression["local"]),
        readExpression(cache, expression["sourceExpr"]),
        expression["localName"]);
  } else if (type == "MemberStore") {
    return new MemberStore(
        readExpression(cache, expression["structExpr"]),
        readReference(cache, expression["structType"]),
        expression["memberIndex"],
        readExpression(cache, expression["sourceExpr"]),
        readReference(cache, expression["resultType"]),
        expression["memberName"]);
  } else if (type == "Discard") {
    return new Discard(
        readExpression(cache, expression["sourceExpr"]),
        readReference(cache, expression["sourceResultType"]));
  } else if (type == "Argument") {
    return new Argument(
        readReference(cache, expression["resultType"]),
        expression["argumentIndex"]);
  } else if (type == "Unstackify") {
    return new Unstackify(
        readLocal(cache, expression["local"]));
  } else if (type == "LocalLoad") {
    return new LocalLoad(
        readLocal(cache, expression["local"]),
        readOwnership(cache, expression["targetOwnership"]),
        expression["localName"]);
  } else if (type == "WeakAlias") {
    return new WeakAlias(
        readExpression(cache, expression["sourceExpr"]),
        readReference(cache, expression["sourceType"]),
        readReferend(cache, expression["sourceReferend"]));
  } else if (type == "Call") {
    return new Call(
        readPrototype(cache, expression["function"]),
        readArray(cache, expression["argExprs"], readExpression));
  } else if (type == "ExternCall") {
    return new ExternCall(
        readPrototype(cache, expression["function"]),
        readArray(cache, expression["argExprs"], readExpression),
        readArray(cache, expression["argTypes"], readReference));
  } else if (type == "Consecutor") {
    return new Consecutor(
        readArray(cache, expression["exprs"], readExpression));
  } else if (type == "Block") {
    return new Block(
        readExpression(cache, expression["innerExpr"]),
        readReference(cache, expression["innerType"]));
  } else if (type == "If") {
    return new If(
        readExpression(cache, expression["conditionBlock"]),
        readExpression(cache, expression["thenBlock"]),
        readReference(cache, expression["thenResultType"]),
        readExpression(cache, expression["elseBlock"]),
        readReference(cache, expression["elseResultType"]),
        readReference(cache, expression["commonSupertype"]));
  } else if (type == "While") {
    return new While(
        readExpression(cache, expression["bodyBlock"]));
  } else if (type == "NewStruct") {
    return new NewStruct(
        readArray(cache, expression["sourceExprs"], readExpression),
        readReference(cache, expression["resultType"]));
  } else if (type == "Destroy") {
    return new Destroy(
        readExpression(cache, expression["structExpr"]),
        readReference(cache, expression["structType"]),
        readArray(cache, expression["localTypes"], readReference),
        readArray(cache, expression["localIndices"], readLocal));
  } else if (type == "MemberLoad") {
    return new MemberLoad(
        readExpression(cache, expression["structExpr"]),
        readStructReferend(cache, expression["structId"]),
        readReference(cache, expression["structType"]),
        expression["memberIndex"],
        readOwnership(cache, expression["targetOwnership"]),
        readReference(cache, expression["expectedMemberType"]),
        readReference(cache, expression["expectedResultType"]),
        expression["memberName"]);
  } else if (type == "NewArrayFromValues") {
    return new NewArrayFromValues(
        readArray(cache, expression["sourceExprs"], readExpression),
        readReference(cache, expression["resultType"]),
        readKnownSizeArray(cache, expression["resultReferend"]));
  } else if (type == "KnownSizeArrayLoad") {
    return new KnownSizeArrayLoad(
        readExpression(cache, expression["arrayExpr"]),
        readReference(cache, expression["arrayType"]),
        readKnownSizeArray(cache, expression["arrayReferend"]),
        readExpression(cache, expression["indexExpr"]),
        readReference(cache, expression["resultType"]),
        readOwnership(cache, expression["targetOwnership"]));
  } else if (type == "UnknownSizeArrayLoad") {
    return new UnknownSizeArrayLoad(
        readExpression(cache, expression["arrayExpr"]),
        readReference(cache, expression["arrayType"]),
        readUnknownSizeArray(cache, expression["arrayReferend"]),
        readExpression(cache, expression["indexExpr"]),
        readReference(cache, expression["indexType"]),
        readReferend(cache, expression["indexReferend"]),
        readReference(cache, expression["resultType"]),
        readOwnership(cache, expression["targetOwnership"]));
  } else if (type == "UnknownSizeArrayStore") {
    return new UnknownSizeArrayStore(
        readExpression(cache, expression["arrayExpr"]),
        readReference(cache, expression["arrayType"]),
        readUnknownSizeArray(cache, expression["arrayReferend"]),
        readExpression(cache, expression["indexExpr"]),
        readReference(cache, expression["indexType"]),
        readReferend(cache, expression["indexReferend"]),
        readExpression(cache, expression["sourceExpr"]),
        readReference(cache, expression["sourceType"]),
        readReferend(cache, expression["sourceReferend"]));
  } else if (type == "ConstructUnknownSizeArray") {
    return new ConstructUnknownSizeArray(
        readExpression(cache, expression["sizeExpr"]),
        readReference(cache, expression["sizeType"]),
        readReferend(cache, expression["sizeReferend"]),
        readExpression(cache, expression["generatorExpr"]),
        readReference(cache, expression["generatorType"]),
        readInterfaceReferend(cache, expression["generatorReferend"]),
        readReference(cache, expression["resultType"]));
  } else if (type == "DestroyUnknownSizeArray") {
    return new DestroyUnknownSizeArray(
        readExpression(cache, expression["arrayExpr"]),
        readReference(cache, expression["arrayType"]),
        readUnknownSizeArray(cache, expression["arrayReferend"]),
        readExpression(cache, expression["consumerExpr"]),
        readReference(cache, expression["consumerType"]),
        readInterfaceReferend(cache, expression["consumerReferend"]));
  } else if (type == "ArrayLength") {
    return new ArrayLength(
        readExpression(cache, expression["sourceExpr"]),
        readReference(cache, expression["sourceType"]));
  } else if (type == "StructToInterfaceUpcast") {
    return new StructToInterfaceUpcast(
        readExpression(cache, expression["sourceExpr"]),
        readReference(cache, expression["sourceStructType"]),
        readStructReferend(cache, expression["sourceStructReferend"]),
        readReference(cache, expression["targetInterfaceType"]),
        readInterfaceReferend(cache, expression["targetInterfaceReferend"]));
  } else if (type == "DestroyKnownSizeArrayIntoFunction") {
    return new DestroyKnownSizeArrayIntoFunction(
        readExpression(cache, expression["arrayExpr"]),
        readReference(cache, expression["arrayType"]),
        readKnownSizeArray(cache, expression["arrayReferend"]),
        readExpression(cache, expression["consumerExpr"]),
        readReference(cache, expression["consumerType"]));
  } else if (type == "InterfaceCall") {
    return new InterfaceCall(
        readArray(cache, expression["argExprs"], readExpression),
        expression["virtualParamIndex"],
        readInterfaceReferend(cache, expression["interfaceRef"]),
        expression["indexInEdge"],
        readPrototype(cache, expression["functionType"]));
  } else if (type == "ConstantStr") {
    return new ConstantStr(
        expression["value"]);
  } else if (type == "LockWeak") {
    return new LockWeak(
        readExpression(cache, expression["sourceExpr"]),
        readReference(cache, expression["sourceType"]),
        readPrototype(cache, expression["someConstructor"]),
        readReference(cache, expression["someType"]),
        readStructReferend(cache, expression["someReferend"]),
        readPrototype(cache, expression["noneConstructor"]),
        readReference(cache, expression["noneType"]),
        readStructReferend(cache, expression["noneReferend"]),
        readReference(cache, expression["resultOptType"]),
        readInterfaceReferend(cache, expression["resultOptReferend"]));
  } else {
    std::cerr << "Unexpected instruction: " << type << std::endl;
    assert(false);
  }
}

StructMember* readStructMember(MetalCache* cache, const json& struuct) {
  assert(struuct.is_object());
  assert(struuct[""] == "StructMember");
  return new StructMember(
      struuct["name"],
      readVariability(struuct["variability"]),
      readReference(cache, struuct["type"]));
}

InterfaceMethod* readInterfaceMethod(MetalCache* cache, const json& struuct) {
  assert(struuct.is_object());
  assert(struuct[""] == "InterfaceMethod");
  return new InterfaceMethod(
      readPrototype(cache, struuct["prototype"]),
      struuct["virtualParamIndex"]);
}

std::pair<InterfaceMethod*, Prototype*> readInterfaceMethodAndPrototypeEntry(MetalCache* cache, const json& edge) {
  assert(edge.is_object());
  assert(edge[""] == "Entry");
  return std::make_pair(
      readInterfaceMethod(cache, edge["key"]),
      readPrototype(cache, edge["value"]));
}

Edge* readEdge(MetalCache* cache, const json& edge) {
  assert(edge.is_object());
  assert(edge[""] == "Edge");
  return new Edge(
      readStructReferend(cache, edge["structName"]),
      readInterfaceReferend(cache, edge["interfaceName"]),
      readArray(cache, edge["methods"], readInterfaceMethodAndPrototypeEntry));
}

StructDefinition* readStruct(MetalCache* cache, const json& struuct) {
  assert(struuct.is_object());
  assert(struuct[""] == "Struct");
  return new StructDefinition(
      readName(cache, struuct["name"]),
      readMutability(struuct["mutability"]),
      readArray(cache, struuct["edges"], readEdge),
      readArray(cache, struuct["members"], readStructMember),
      struuct["weakable"]);
}

InterfaceDefinition* readInterface(MetalCache* cache, const json& struuct) {
  assert(struuct.is_object());
  assert(struuct[""] == "Interface");
  return new InterfaceDefinition(
      readName(cache, struuct["name"]),
      readMutability(struuct["mutability"]),
      {},
      readArray(cache, struuct["methods"], readInterfaceMethod),
      struuct["weakable"]);
}

Function* readFunction(MetalCache* cache, const json& function) {
  assert(function.is_object());
  assert(function[""] == "Function");
  return new Function(
      readPrototype(cache, function["prototype"]),
      readExpression(cache, function["block"]));
}

std::pair<Referend*, Prototype*> readReferendAndPrototypeEntry(MetalCache* cache, const json& edge) {
  assert(edge.is_object());
  assert(edge[""] == "Entry");
  return std::make_pair(
      readReferend(cache, edge["key"]),
      readPrototype(cache, edge["value"]));
}

Program* readProgram(MetalCache* cache, const json& program) {
  assert(program.is_object());
  assert(program[""] == "Program");
  return new Program(
      readArrayIntoMap<std::string, InterfaceDefinition*>(
          cache,
          program["interfaces"],
          [](MetalCache* cache, json j){
            auto s = readInterface(cache, j);
            return std::make_pair(s->name->name, s);
          }),
      readArrayIntoMap<std::string, StructDefinition*>(
          cache,
          program["structs"],
          [](MetalCache* cache, json j){
            auto s = readStruct(cache, j);
            return std::make_pair(s->name->name, s);
          }),
      nullptr,//readStructName(program["emptyPackStructRef"]),
      {},//readArray<readExtern>(program["externs"]),
      readArrayIntoMap<std::string, Function*>(
          cache,
          program["functions"],
          [](MetalCache* cache, json j){
            auto f = readFunction(cache, j);
            return std::make_pair(f->prototype->name->name, f);
          }),
      readArrayIntoMap<Referend*, Prototype*>(
          cache,
          program["immDestructorsByKind"],
          readReferendAndPrototypeEntry));
}
