#include "type_declaration.hpp"
#include "r_api.hpp"
#include "utilities.hpp"

#undef length
#include <limits>
#include <tastr/parser/parser.hpp>
#include <map>
#include <cassert>

/* the keys are unique numbers, map is faster compared to unordered map  */
std::map<const tastr::ast::Node*, std::string> type_representation_cache;

using packdecl_t =
    std::pair<std::string, std::unique_ptr<tastr::ast::TopLevelNode>>;

static std::vector<packdecl_t> type_declaration_cache;

using index_t = int;
const index_t INVALID_INDEX = -1;

bool is_invalid_index(index_t index) {
    return index == INVALID_INDEX;
}

bool is_valid_index(index_t index) {
    return !is_invalid_index(index);
}

index_t get_package_index(const std::string& package_name) {
    index_t size = type_declaration_cache.size();
    for (index_t i = 0; i < size; ++i) {
        if (type_declaration_cache[i].first == package_name) {
            return i;
        }
    }
    return INVALID_INDEX;
}

const std::string& get_package_name(index_t package_index) {
    return type_declaration_cache[package_index].first;
}

index_t get_function_index(index_t package_index,
                           const std::string& function_name) {
    if (is_invalid_index(package_index)) {
        return INVALID_INDEX;
    }

    const std::unique_ptr<tastr::ast::TopLevelNode>& top_level_node =
        type_declaration_cache[package_index].second;

    index_t size = top_level_node->size();

    for (index_t index = 0; index < size; ++index) {
        const std::string& name =
            top_level_node->at(index).get_identifier().get_name();
        if (name == function_name) {
            return index;
        }
    }

    return INVALID_INDEX;
}

const std::string& get_function_name(index_t package_index,
                                     index_t function_index) {
    return type_declaration_cache[package_index]
        .second->at(function_index)
        .get_identifier()
        .get_name();
}

SEXP r_get_type_index(SEXP pkg_name, SEXP fun_name) {
    const std::string package_name = CHAR(asChar(pkg_name));
    index_t package_index = get_package_index(package_name);

    const std::string function_name = CHAR(asChar(fun_name));
    index_t function_index = get_function_index(package_index, function_name);

    SEXP type_index = PROTECT(allocVector(INTSXP, 2));

    INTEGER(type_index)[0] = package_index;
    INTEGER(type_index)[1] = function_index;

    UNPROTECT(1);

    return type_index;
}

SEXP r_get_typed_package_names() {
    int size = type_declaration_cache.size();

    SEXP result = PROTECT(allocVector(STRSXP, size));

    for (int i = 0; i < size; ++i) {
        const std::string& package_name = type_declaration_cache[i].first;
        SET_STRING_ELT(result, i, mkChar(package_name.c_str()));
    }

    UNPROTECT(1);

    return result;
}

SEXP r_get_typed_function_names(SEXP pkg_name) {
    const std::string package_name = CHAR(asChar(pkg_name));

    index_t package_index = get_package_index(package_name);

    if (is_invalid_index(package_index)) {
        return allocVector(STRSXP, 0);
    }

    const std::unique_ptr<tastr::ast::TopLevelNode>& top_level_node =
        type_declaration_cache[package_index].second;

    int size = top_level_node->size();

    SEXP result = PROTECT(allocVector(STRSXP, size));

    for (int index = 0; index < size; ++index) {
        const std::string& function_name =
            top_level_node->at(index).get_identifier().get_name();
        SET_STRING_ELT(result, index, mkChar(function_name.c_str()));
    }

    UNPROTECT(1);

    return result;
}

SEXP r_is_package_typed(SEXP pkg_name) {
    const std::string package_name = CHAR(asChar(pkg_name));
    index_t index = get_package_index(package_name);

    return is_valid_index(index) ? R_TrueValue : R_FalseValue;
}

SEXP r_is_function_typed(SEXP pkg_name, SEXP fun_name) {
    const std::string package_name = CHAR(asChar(pkg_name));
    index_t package_index = get_package_index(package_name);

    const std::string function_name = CHAR(asChar(fun_name));
    index_t function_index = get_function_index(package_index, function_name);

    return is_valid_index(function_index) ? R_TrueValue : R_FalseValue;
}

SEXP r_is_type_well_formed(SEXP r_type) {
    std::string type = CHAR(asChar(r_type));

    tastr::parser::ParseResult result(tastr::parser::parse_string(type));

    SEXP status = PROTECT(ScalarLogical(static_cast<bool>(result)));
    SEXP message = PROTECT(mkString(result.get_error_message().c_str()));
    SEXP location = PROTECT(mkString(to_string(result.get_error_location()).c_str()));
    SEXP list = PROTECT(create_list({status, message, location},
                                    {"status", "message", "location"}));

    UNPROTECT(4);

    return list;
}

SEXP r_import_type_declarations(SEXP pkg_name, SEXP typedecl_filepath) {
    const std::string package_name = CHAR(asChar(pkg_name));

    std::string package_typedecl_filepath(CHAR(asChar(typedecl_filepath)));

    tastr::parser::ParseResult result(
        tastr::parser::parse_file(package_typedecl_filepath));

    /* return empty vector if type declaration file is not well formed  */
    if (!result) {
        errorcall(R_NilValue,
                  "%s in '%s' at %s",
                  result.get_error_message().c_str(),
                  package_typedecl_filepath.c_str(),
                  to_string(result.get_error_location()).c_str());
        return allocVector(STRSXP, 0);
    }

    SEXP function_names =
        PROTECT(allocVector(STRSXP, result.get_top_level_node()->size()));

    int size = result.get_top_level_node()->size();

    for (int index = 0; index < size; ++index) {
        const tastr::ast::TypeDeclarationNode& decl =
            result.get_top_level_node()->at(index);
        std::string name(decl.get_identifier().get_name());
        SET_STRING_ELT(function_names, index, mkChar(name.c_str()));
    }

    packdecl_t package_type_declaration =
        std::make_pair(package_name, std::move(result.get_top_level_node()));

    type_declaration_cache.push_back(std::move(package_type_declaration));

    UNPROTECT(1);

    return function_names;
}

SEXP r_set_type_declaration(SEXP pkg_name, SEXP fun_name, SEXP type_decl) {
    const std::string package_name = CHAR(asChar(pkg_name));
    const std::string function_name = CHAR(asChar(fun_name));
    const std::string type_declaration = CHAR(asChar(type_decl));

    tastr::parser::ParseResult result(
        tastr::parser::parse_string(type_declaration));

    if (!result) {
        errorcall(R_NilValue,
                  "%s in '%s' at %s",
                  result.get_error_message().c_str(),
                  type_declaration.c_str(),
                  to_string(result.get_error_location()).c_str());
        return R_NilValue;
    }

    index_t package_index = get_package_index(package_name);

    index_t function_index = get_function_index(package_index, function_name);

    std::unique_ptr<tastr::ast::TopLevelNode> pack_node =
        std::move(result.get_top_level_node());

    /* package is not present; add it */
    if (is_invalid_index(package_index)) {
        packdecl_t packdecl =
            std::make_pair(package_name, std::move(pack_node));

        type_declaration_cache.push_back(std::move(packdecl));

    }
    /* package is present but function is not  */
    else if (is_invalid_index(function_index)) {
        std::unique_ptr<tastr::ast::TypeDeclarationNode> fun_node =
            std::move(pack_node->get_type_declarations()[0]);

        type_declaration_cache[package_index]
            .second->get_type_declarations()
            .push_back(std::move(fun_node));
    }
    /* package and function, both are present  */
    else {
        std::unique_ptr<tastr::ast::TypeDeclarationNode> fun_node =
            std::move(pack_node->get_type_declarations()[0]);

        type_declaration_cache[package_index]
            .second->get_type_declarations()[function_index] =
            std::move(fun_node);
    }

    return R_NilValue;
}

void show_function_type_declaration_(index_t package_index,
                                     index_t function_index,
                                     bool style) {
    assert(is_valid_index(package_index));
    assert(is_valid_index(function_index));

    const tastr::ast::TypeDeclarationNode& node =
        type_declaration_cache[package_index].second->at(function_index);

    std::cout << std::endl;
    tastr::parser::unparse_stdout(node, false, style);
    std::cout << std::endl;
}

SEXP r_show_function_type_declaration(SEXP pkg_name,
                                      SEXP fun_name,
                                      SEXP style) {
    const std::string package_name = CHAR(asChar(pkg_name));
    const std::string function_name = CHAR(asChar(fun_name));

    index_t package_index = get_package_index(package_name);
    index_t function_index = get_function_index(package_index, function_name);

    /*  function not found */
    if (is_invalid_index(package_index)) {
        errorcall(R_NilValue,
                  "type declarations not available for '%s::%s'",
                  package_name.c_str(),
                  function_name.c_str());
    }
    /* function found  */
    else {
        show_function_type_declaration_(
            package_index, function_index, asLogical(style));
    }

    return R_NilValue;
}

void show_package_type_declarations_(index_t package_index, bool style) {
    assert(is_valid_index(package_index));

    const std::string& package_name =
        type_declaration_cache[package_index].first;

    const tastr::ast::TopLevelNode& node =
        *type_declaration_cache[package_index].second.get();

    std::cout << std::string(80, '#') << std::endl;
    std::cout << "## " << package_name << std::endl;
    std::cout << std::string(80, '#') << std::endl;

    std::cout << std::endl;
    tastr::parser::unparse_stdout(node, false, style);
    std::cout << std::endl;
}

SEXP r_show_package_type_declarations(SEXP pkg_name, SEXP style) {
    const std::string package_name = CHAR(asChar(pkg_name));
    index_t package_index = get_package_index(package_name);

    /*  package not found */
    if (is_invalid_index(package_index)) {
        errorcall(R_NilValue,
                  "type declarations not available for package '%s'",
                  package_name.c_str());

    }
    /* package found  */
    else {
        show_package_type_declarations_(package_index, asLogical(style));
    }

    return R_NilValue;
}

SEXP r_show_type_declarations(SEXP style) {
    for (int i = 0; i < type_declaration_cache.size(); ++i) {
        show_package_type_declarations_(i, asLogical(style));
    }
    return R_NilValue;
}

int get_function_parameter_count(
    const tastr::ast::FunctionTypeNode* function_type) {
    const tastr::ast::Node& node = function_type->get_parameter();

    if (node.is_parameter_node()) {
        return tastr::ast::as<tastr::ast::ParameterNode>(node)
            .get_parameter_count();
    } else {
        return std::numeric_limits<int>::max();
    }
}

const tastr::ast::Node&
get_function_parameter_type(const tastr::ast::FunctionTypeNode* function_type,
                            int formal_parameter_position) {
    /* NOTE: this cannot be null as get_function_type never returns null */
    const tastr::ast::Node& node = function_type->get_parameter();

    if (node.is_any_type_node()) {
        return node;
    }

    else {
        const tastr::ast::ParameterNode& parameter_node =
            tastr::ast::as<tastr::ast::ParameterNode>(node);

        int parameter_count = parameter_node.get_parameter_count();

        if (formal_parameter_position >= parameter_count) {
            errorcall(R_NilValue,
                      "type for parameter %d requested for function with type "
                      "%s and %d parameters",
                      /* NOTE: indexing starts from 1 in R */
                      formal_parameter_position + 1,
                      type_to_string(*function_type).c_str(),
                      parameter_count);
        }

        return parameter_node.at(formal_parameter_position);
    }
}

const tastr::ast::Node&
get_function_return_type(const tastr::ast::FunctionTypeNode* function_type) {
    /* NOTE: this cannot be null as get_function_type never returns null */
    return function_type->get_return_type();
}

const tastr::ast::FunctionTypeNode* get_function_type(int package_index,
                                                      int function_index) {
    if (is_invalid_index(function_index)) {
        errorcall(R_NilValue,
                  "type declaration not available for '%s::%s'",
                  get_package_name(package_index).c_str(),
                  get_function_name(package_index, function_index).c_str());
        exit(1);
    }

    const tastr::ast::TypeNode& node = type_declaration_cache[package_index]
                                           .second->at(function_index)
                                           .get_type();

    if (node.is_function_type_node()) {
        return &tastr::ast::as<tastr::ast::FunctionTypeNode>(node);
    } else {
        errorcall(R_NilValue,
                  "'%s::%s' has type %s which is not a function type",
                  get_package_name(package_index).c_str(),
                  get_function_name(package_index, function_index).c_str(),
                  type_to_string(node).c_str());
        exit(1);
    }
}

const std::string& type_to_string(const tastr::ast::Node& node) {
    auto iter = type_representation_cache.find(&node);
    /*  node is cached */
    if (iter != type_representation_cache.end()) {
        return iter->second;
    }
    /* node is not cached; cache and return */
    else {
        auto iter2 =
            type_representation_cache
                .insert({&node, std::move(tastr::parser::to_string(node))})
                .first;
        return iter2->second;
    }
}

static void type_finalizer(SEXP type)
{
    if (NULL == R_ExternalPtrAddr(type)) {
        return;
    }
    auto node = (tastr::ast::Node*) R_ExternalPtrAddr(type);
    delete node;
    R_ClearExternalPtr(type);
}

static SEXP node2extptr(tastr::ast::Node* node) {
    SEXP type = PROTECT(R_MakeExternalPtr(node, R_NilValue, R_NilValue));
    Rf_setAttrib(type, R_ClassSymbol, Rf_mkString("tastr"));
    R_RegisterCFinalizerEx(type, type_finalizer, TRUE);
    UNPROTECT(1);
    return type;
}

SEXP r_parse_type(SEXP str) {
    std::string s = CHAR(STRING_ELT(str, 0));
    auto res = tastr::parser::parse_string(s);
    if (!res) {
        Rf_error("Unable to parse `%s`: %s", s.c_str(), res.get_error_message().c_str());
        return R_NilValue;
    }
    
    if (res.get_top_level_node()->size() != 1) {
        Rf_error("Parsed %d types, expected 1", res.get_top_level_node()->size());
        return R_NilValue;
    }

    tastr::ast::TypeNode* type = res.get_top_level_node()->at(0).get_type().clone().release();

    return node2extptr(type);
}

// Will return true if type1 <: type2
// Note: Transitive subtyping rules are directly encoded.
// E.g., int <: clx is encoded directly, while in actuality int <: dbl <: clx
// TODO: Make this function not a disgusting monolith.
SEXP r_is_subtype_inner(const tastr::ast::Node* node1, const tastr::ast::Node* node2) {
    // This will be called from R, so type1 and type2 are going to be external pointers
    // that were created through `node2extptr`. So, get the nodes associated with them.

    // TODO: If we want this to be reflexive, the code will require significant changes. 
    // ! Note: The handling of `any` is explicitly not reflexive. 
    // if (*node1 == *node2) {
    //     return R_TrueValue;
    // }

    // TODO: Currently the nullable type is not implemented in the grammar.
    if (node2->get_kind() == tastr::ast::Node::Kind::NullableType) {
        // T1 <: ? T2 iff T1 <: T2
        auto node2_inner = &((tastr::ast::NullableTypeNodePtr) node2)->get_inner_type();
        r_is_subtype_inner(node1, node2_inner);
    }

    // T <: * \forall T
    if (node2->get_kind() == tastr::ast::Node::Kind::AnyType && node1->get_kind() != tastr::ast::Node::Kind::AnyType) {
        return R_TrueValue;
    }

    // list<T> <: list<T'> if T <: T'
    if (node1->get_kind() == tastr::ast::Node::Kind::ListType &&
        node2->get_kind() == tastr::ast::Node::Kind::ListType) {
        // There should be a single type node inside of these.
        auto node1__params = &((tastr::ast::ListTypeNodePtr) node1)->get_parameters();
        auto node2__params = &((tastr::ast::ListTypeNodePtr) node2)->get_parameters();
        if (node1__params->get_parameter_count() == 1 && node2__params->get_parameter_count() == 1) 
            return r_is_subtype_inner(&node1__params->at(0), &node2__params->at(0));
    }

    auto isItASubtype = R_FalseValue;    
    switch (node1->get_kind()) {
    case tastr::ast::Node::Kind::CharacterAScalarType: // chr
        switch (node2->get_kind()) {
        case tastr::ast::Node::Kind::VectorType:    // _[]
            switch (((tastr::ast::VectorTypeNode*) node2)->get_scalar_type().get_kind()) {
            case tastr::ast::Node::Kind::NAScalarType:
                if (((tastr::ast::NAScalarTypeNode*) &((tastr::ast::VectorTypeNode*) node2)->get_scalar_type())->get_a_scalar_type().get_kind() == tastr::ast::Node::Kind::CharacterAScalarType)
                    isItASubtype = R_TrueValue;
                break;
            case tastr::ast::Node::Kind::CharacterAScalarType:    // chr <: chr[]
                isItASubtype = R_TrueValue;
            }
            break;
        case tastr::ast::Node::Kind::NAScalarType:  // chr <: ^chr
            if (((tastr::ast::NAScalarTypeNode*) node2)->get_a_scalar_type().get_kind() == tastr::ast::Node::Kind::CharacterAScalarType)
                isItASubtype = R_TrueValue;
            break;
        }
        break;
    case tastr::ast::Node::Kind::ComplexAScalarType: // clx
        switch (node2->get_kind()) {
        case tastr::ast::Node::Kind::VectorType:    // _[]
            switch (((tastr::ast::VectorTypeNode*) node2)->get_scalar_type().get_kind()) {
            case tastr::ast::Node::Kind::NAScalarType:
                if (((tastr::ast::NAScalarTypeNode*) &((tastr::ast::VectorTypeNode*) node2)->get_scalar_type())->get_a_scalar_type().get_kind() == tastr::ast::Node::Kind::ComplexAScalarType)
                    isItASubtype = R_TrueValue;
                break;
            case tastr::ast::Node::Kind::ComplexAScalarType:    // clx <: clx[]
                isItASubtype = R_TrueValue;
            }
            break;
        case tastr::ast::Node::Kind::NAScalarType: // clx <: ^clx
            if (((tastr::ast::NAScalarTypeNode*) node2)->get_a_scalar_type().get_kind() == tastr::ast::Node::Kind::ComplexAScalarType)
                isItASubtype = R_TrueValue;
            break;
        }
        break;
    case tastr::ast::Node::Kind::DoubleAScalarType: // dbl
        switch (node2->get_kind()) {
        case tastr::ast::Node::Kind::VectorType:    // _[]
            switch (((tastr::ast::VectorTypeNode*) node2)->get_scalar_type().get_kind()) {
            case tastr::ast::Node::Kind::NAScalarType: {
                auto node2__kind = ((tastr::ast::NAScalarTypeNode*) &((tastr::ast::VectorTypeNode*) node2)->get_scalar_type())->get_a_scalar_type().get_kind();
                if (node2__kind == tastr::ast::Node::Kind::DoubleAScalarType || // dbl <: ^dbl
                    node2__kind == tastr::ast::Node::Kind::ComplexAScalarType)  // dbl <: ^clx
                    isItASubtype = R_TrueValue;
                break;
            }    
            case tastr::ast::Node::Kind::DoubleAScalarType:     // dbl <: dbl[]
            case tastr::ast::Node::Kind::ComplexAScalarType:    // dbl <: clx[]
                isItASubtype = R_TrueValue;
            }
            break;
        case tastr::ast::Node::Kind::NAScalarType: {
            auto node2__kind = ((tastr::ast::NAScalarTypeNode*) node2)->get_a_scalar_type().get_kind();
            if (node2__kind == tastr::ast::Node::Kind::DoubleAScalarType || // dbl <: ^dbl
                node2__kind == tastr::ast::Node::Kind::ComplexAScalarType)  // dbl <: ^clx
                isItASubtype = R_TrueValue;
            break;
        }
        case tastr::ast::Node::Kind::ComplexAScalarType:
            isItASubtype = R_TrueValue;
            break;
        }
        break;
    case tastr::ast::Node::Kind::IntegerAScalarType: // int
        switch (node2->get_kind()) {
        case tastr::ast::Node::Kind::VectorType:     // _[]
            switch (((tastr::ast::VectorTypeNode*) node2)->get_scalar_type().get_kind()) {
            case tastr::ast::Node::Kind::NAScalarType: {
                auto node2__kind = ((tastr::ast::NAScalarTypeNode*) &((tastr::ast::VectorTypeNode*) node2)->get_scalar_type())->get_a_scalar_type().get_kind();
                if (node2__kind == tastr::ast::Node::Kind::IntegerAScalarType ||// int <: ^int
                    node2__kind == tastr::ast::Node::Kind::DoubleAScalarType || // int <: ^dbl
                    node2__kind == tastr::ast::Node::Kind::ComplexAScalarType)  // int <: ^clx
                    isItASubtype = R_TrueValue;
                break;
            }
            case tastr::ast::Node::Kind::IntegerAScalarType:    // int <: int[]
            case tastr::ast::Node::Kind::DoubleAScalarType:     // int <: dbl[]
            case tastr::ast::Node::Kind::ComplexAScalarType:    // int <: clx[]
                isItASubtype = R_TrueValue;
                break;
            }
            break;
        case tastr::ast::Node::Kind::NAScalarType: {
            auto node2__kind = ((tastr::ast::NAScalarTypeNode*) node2)->get_a_scalar_type().get_kind();
            if (node2__kind == tastr::ast::Node::Kind::IntegerAScalarType ||// int <: ^int
                node2__kind == tastr::ast::Node::Kind::DoubleAScalarType || // int <: ^dbl
                node2__kind == tastr::ast::Node::Kind::ComplexAScalarType)  // int <: ^clx
                isItASubtype = R_TrueValue;
            break;
        }
        case tastr::ast::Node::Kind::DoubleAScalarType:
        case tastr::ast::Node::Kind::ComplexAScalarType:
            isItASubtype = R_TrueValue;
        }
        break;
    case tastr::ast::Node::Kind::LogicalAScalarType: // lgl
        switch (node2->get_kind()) {
        case tastr::ast::Node::Kind::VectorType:     // _[]
            switch (((tastr::ast::VectorTypeNode*) node2)->get_scalar_type().get_kind()) {
            case tastr::ast::Node::Kind::NAScalarType: {
                auto node2__kind = ((tastr::ast::NAScalarTypeNode*) &((tastr::ast::VectorTypeNode*) node2)->get_scalar_type())->get_a_scalar_type().get_kind();
                if (node2__kind == tastr::ast::Node::Kind::LogicalAScalarType || // lgl <: ^lgl
                    node2__kind == tastr::ast::Node::Kind::IntegerAScalarType || // lgl <: ^int
                    node2__kind == tastr::ast::Node::Kind::DoubleAScalarType ||  // lgl <: ^dbl
                    node2__kind == tastr::ast::Node::Kind::ComplexAScalarType)   // lgl <: ^clx
                    isItASubtype = R_TrueValue;
                break;
            }
            case tastr::ast::Node::Kind::LogicalAScalarType:    // lgl <: lgl[]
            case tastr::ast::Node::Kind::IntegerAScalarType:    // lgl <: int[]
            case tastr::ast::Node::Kind::DoubleAScalarType:     // lgl <: dbl[]
            case tastr::ast::Node::Kind::ComplexAScalarType:    // lgl <: clx[]
                isItASubtype = R_TrueValue;
            }
            break;
        case tastr::ast::Node::Kind::NAScalarType: {
            auto node2__kind = ((tastr::ast::NAScalarTypeNode*) node2)->get_a_scalar_type().get_kind();
            if (node2__kind == tastr::ast::Node::Kind::LogicalAScalarType || // lgl <: ^lgl
                node2__kind == tastr::ast::Node::Kind::IntegerAScalarType || // lgl <: ^int
                node2__kind == tastr::ast::Node::Kind::DoubleAScalarType  || // lgl <: ^dbl
                node2__kind == tastr::ast::Node::Kind::ComplexAScalarType)   // lgl <: ^clx
                isItASubtype = R_TrueValue;
            break;
        }
        case tastr::ast::Node::Kind::IntegerAScalarType:
        case tastr::ast::Node::Kind::DoubleAScalarType:
        case tastr::ast::Node::Kind::ComplexAScalarType:
            isItASubtype = R_TrueValue;
            break;
        }
        break;
    case tastr::ast::Node::Kind::NAScalarType: { // ^_
        auto node1__inner_kind = ((tastr::ast::NAScalarTypeNode*) node1)->get_a_scalar_type().get_kind();
        switch(node1__inner_kind) {
            case tastr::ast::Node::Kind::CharacterAScalarType:
                switch (node2->get_kind()) {
                case tastr::ast::Node::Kind::VectorType:    // _[]
                    switch (((tastr::ast::VectorTypeNode*) node2)->get_scalar_type().get_kind()) {
                    case tastr::ast::Node::Kind::NAScalarType:
                        if (((tastr::ast::NAScalarTypeNode*) &((tastr::ast::VectorTypeNode*) node2)->get_scalar_type())->get_a_scalar_type().get_kind() == tastr::ast::Node::Kind::CharacterAScalarType)
                            isItASubtype = R_TrueValue;
                        break;
                    }
                    break;
                }
                break;
            case tastr::ast::Node::Kind::ComplexAScalarType:
                switch (node2->get_kind()) {
                case tastr::ast::Node::Kind::VectorType:    // _[]
                    switch (((tastr::ast::VectorTypeNode*) node2)->get_scalar_type().get_kind()) {
                    case tastr::ast::Node::Kind::NAScalarType:
                        if (((tastr::ast::NAScalarTypeNode*) &((tastr::ast::VectorTypeNode*) node2)->get_scalar_type())->get_a_scalar_type().get_kind() == tastr::ast::Node::Kind::ComplexAScalarType)
                            isItASubtype = R_TrueValue;
                        break;
                    }
                    break;
                }
                break;
            case tastr::ast::Node::Kind::DoubleAScalarType:
                switch (node2->get_kind()) {
                case tastr::ast::Node::Kind::VectorType:    // _[]
                    switch (((tastr::ast::VectorTypeNode*) node2)->get_scalar_type().get_kind()) {
                    case tastr::ast::Node::Kind::NAScalarType: {
                        auto node2__kind = ((tastr::ast::NAScalarTypeNode*) &((tastr::ast::VectorTypeNode*) node2)->get_scalar_type())->get_a_scalar_type().get_kind();
                        if (node2__kind == tastr::ast::Node::Kind::DoubleAScalarType || // dbl <: ^dbl
                            node2__kind == tastr::ast::Node::Kind::ComplexAScalarType)  // dbl <: ^clx
                            isItASubtype = R_TrueValue;
                        break;
                    }    
                    }
                }
                break;
            case tastr::ast::Node::Kind::IntegerAScalarType:
                switch (node2->get_kind()) {
                case tastr::ast::Node::Kind::VectorType:     // _[]
                    switch (((tastr::ast::VectorTypeNode*) node2)->get_scalar_type().get_kind()) {
                    case tastr::ast::Node::Kind::NAScalarType: {
                        auto node2__kind = ((tastr::ast::NAScalarTypeNode*) &((tastr::ast::VectorTypeNode*) node2)->get_scalar_type())->get_a_scalar_type().get_kind();
                        if (node2__kind == tastr::ast::Node::Kind::IntegerAScalarType ||// int <: ^int
                            node2__kind == tastr::ast::Node::Kind::DoubleAScalarType || // int <: ^dbl
                            node2__kind == tastr::ast::Node::Kind::ComplexAScalarType)  // int <: ^clx
                            isItASubtype = R_TrueValue;
                        break;
                    }
                    }
                }
                break;
            case tastr::ast::Node::Kind::LogicalAScalarType:
                switch (node2->get_kind()) {
                case tastr::ast::Node::Kind::VectorType:     // _[]
                    switch (((tastr::ast::VectorTypeNode*) node2)->get_scalar_type().get_kind()) {
                    case tastr::ast::Node::Kind::NAScalarType: {
                        auto node2__kind = ((tastr::ast::NAScalarTypeNode*) &((tastr::ast::VectorTypeNode*) node2)->get_scalar_type())->get_a_scalar_type().get_kind();
                        if (node2__kind == tastr::ast::Node::Kind::LogicalAScalarType || // lgl <: ^lgl
                            node2__kind == tastr::ast::Node::Kind::IntegerAScalarType || // lgl <: ^int
                            node2__kind == tastr::ast::Node::Kind::DoubleAScalarType ||  // lgl <: ^dbl
                            node2__kind == tastr::ast::Node::Kind::ComplexAScalarType)   // lgl <: ^clx
                            isItASubtype = R_TrueValue;
                        break;
                    }
                    }
                }
                break;
        default:
            break;
        }
        break;
    }
    // _[] <: 
    case tastr::ast::Node::Kind::VectorType: {
            switch (node2->get_kind()) {
                // It should be a vector, too.
                case tastr::ast::Node::Kind::VectorType: {
                    // What's the vector type for node1?
                    switch (((tastr::ast::VectorTypeNode*) node1)->get_scalar_type().get_kind()) {
                        case tastr::ast::Node::Kind::CharacterAScalarType:
                            // chr[] <: ^chr[]
                            switch (((tastr::ast::VectorTypeNode*) node2)->get_scalar_type().get_kind()) {
                                case tastr::ast::Node::Kind::NAScalarType: {
                                    auto node2__kind = ((tastr::ast::NAScalarTypeNode*) &((tastr::ast::VectorTypeNode*) node2)->get_scalar_type())->get_a_scalar_type().get_kind();
                                    if (node2__kind == tastr::ast::Node::Kind::CharacterAScalarType) {
                                        isItASubtype = R_TrueValue;
                                    }
                                    break;
                                }
                            }
                            break;
                        case tastr::ast::Node::Kind::ComplexAScalarType:
                            // clx[] <: ^clx[]
                            switch (((tastr::ast::VectorTypeNode*) node2)->get_scalar_type().get_kind()) {
                                case tastr::ast::Node::Kind::NAScalarType: {
                                    auto node2__kind = ((tastr::ast::NAScalarTypeNode*) &((tastr::ast::VectorTypeNode*) node2)->get_scalar_type())->get_a_scalar_type().get_kind();
                                    if (node2__kind == tastr::ast::Node::Kind::ComplexAScalarType) {
                                        isItASubtype = R_TrueValue;
                                    }
                                    break;
                                }
                            }
                            break;
                        case tastr::ast::Node::Kind::DoubleAScalarType:
                            // dbl[] <: {clx[], ^clx[], ^dbl[]}
                            switch (((tastr::ast::VectorTypeNode*) node2)->get_scalar_type().get_kind()) {
                                case tastr::ast::Node::Kind::NAScalarType: {
                                    auto node2__kind = ((tastr::ast::NAScalarTypeNode*) &((tastr::ast::VectorTypeNode*) node2)->get_scalar_type())->get_a_scalar_type().get_kind();
                                    if (node2__kind == tastr::ast::Node::Kind::ComplexAScalarType ||
                                        node2__kind == tastr::ast::Node::Kind::DoubleAScalarType) {
                                        isItASubtype = R_TrueValue;
                                    }
                                    break;
                                }
                                case tastr::ast::Node::Kind::ComplexAScalarType: {
                                    isItASubtype = R_TrueValue;
                                    break;
                                }
                            }
                            break;
                        case tastr::ast::Node::Kind::IntegerAScalarType:
                            // int[] <: {clx[], dbl[], ^clx[], ^dbl[], ^int[]}
                            switch (((tastr::ast::VectorTypeNode*) node2)->get_scalar_type().get_kind()) {
                                case tastr::ast::Node::Kind::NAScalarType: {
                                    auto node2__kind = ((tastr::ast::NAScalarTypeNode*) &((tastr::ast::VectorTypeNode*) node2)->get_scalar_type())->get_a_scalar_type().get_kind();
                                    if (node2__kind == tastr::ast::Node::Kind::ComplexAScalarType ||
                                        node2__kind == tastr::ast::Node::Kind::DoubleAScalarType  || 
                                        node2__kind == tastr::ast::Node::Kind::IntegerAScalarType) {
                                        isItASubtype = R_TrueValue;
                                    }
                                    break;
                                }
                                case tastr::ast::Node::Kind::ComplexAScalarType: 
                                case tastr::ast::Node::Kind::DoubleAScalarType:
                                    isItASubtype = R_TrueValue;
                                    break;
                            }
                            break;
                        case tastr::ast::Node::Kind::LogicalAScalarType:
                            // lgl[] <: {clx[], dbl[], int[], ^clx[], ^dbl[], ^int[], ^lgl[]}
                            switch (((tastr::ast::VectorTypeNode*) node2)->get_scalar_type().get_kind()) {
                                case tastr::ast::Node::Kind::NAScalarType: {
                                    auto node2__kind = ((tastr::ast::NAScalarTypeNode*) &((tastr::ast::VectorTypeNode*) node2)->get_scalar_type())->get_a_scalar_type().get_kind();
                                    if (node2__kind == tastr::ast::Node::Kind::ComplexAScalarType ||
                                        node2__kind == tastr::ast::Node::Kind::DoubleAScalarType  || 
                                        node2__kind == tastr::ast::Node::Kind::IntegerAScalarType ||
                                        node2__kind == tastr::ast::Node::Kind::LogicalAScalarType) {
                                        isItASubtype = R_TrueValue;
                                    }
                                    break;
                                }
                                case tastr::ast::Node::Kind::ComplexAScalarType: 
                                case tastr::ast::Node::Kind::DoubleAScalarType:
                                case tastr::ast::Node::Kind::IntegerAScalarType:
                                    isItASubtype = R_TrueValue;
                                    break;
                            }
                            break;
                        case tastr::ast::Node::Kind::NAScalarType: {
                            // If node2 isn't an NA vector, then they cant be subtypes.
                            if (((tastr::ast::VectorTypeNode*) node2)->get_scalar_type().get_kind() != tastr::ast::Node::Kind::NAScalarType) {
                                break;
                            }
                            auto node1__kind = ((tastr::ast::NAScalarTypeNode*) &((tastr::ast::VectorTypeNode*) node1)->get_scalar_type())->get_a_scalar_type().get_kind();
                            switch (node1__kind) {
                                case tastr::ast::Node::Kind::CharacterAScalarType:
                                    // None.
                                    break;
                                case tastr::ast::Node::Kind::ComplexAScalarType: 
                                    // None.
                                    break;
                                case tastr::ast::Node::Kind::DoubleAScalarType: {
                                    // ^dbl[] <: ^clx[]
                                    auto node2__kind = ((tastr::ast::NAScalarTypeNode*) &((tastr::ast::VectorTypeNode*) node2)->get_scalar_type())->get_a_scalar_type().get_kind();
                                    if (node2__kind == tastr::ast::Node::Kind::ComplexAScalarType)
                                        isItASubtype = R_TrueValue;
                                    break;
                                }
                                case tastr::ast::Node::Kind::IntegerAScalarType: {
                                    // ^int[] <: ^clx[], ^dbl[]
                                    auto node2__kind = ((tastr::ast::NAScalarTypeNode*) &((tastr::ast::VectorTypeNode*) node2)->get_scalar_type())->get_a_scalar_type().get_kind();
                                    if (node2__kind == tastr::ast::Node::Kind::ComplexAScalarType ||
                                        node2__kind == tastr::ast::Node::Kind::DoubleAScalarType)
                                        isItASubtype = R_TrueValue;
                                    break;
                                }
                                case tastr::ast::Node::Kind::LogicalAScalarType: {
                                    // ^lgl[] <: ^clx[], ^dbl[], ^int[]
                                    auto node2__kind = ((tastr::ast::NAScalarTypeNode*) &((tastr::ast::VectorTypeNode*) node2)->get_scalar_type())->get_a_scalar_type().get_kind();
                                    if (node2__kind == tastr::ast::Node::Kind::ComplexAScalarType ||
                                        node2__kind == tastr::ast::Node::Kind::DoubleAScalarType  ||
                                        node2__kind == tastr::ast::Node::Kind::IntegerAScalarType)
                                        isItASubtype = R_TrueValue;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    break;
                }
            }
            break;
        }
    default:
        break;
    }

    return isItASubtype;
}

// Function takes two types, and returns if
bool are_nodes_equal(const tastr::ast::Node* n1, const tastr::ast::Node* n2) {
    // If they are not the same, bye.
    if (n1->get_kind() != n2->get_kind()) {
        return false;
    }

    // Now we know they have the same kind.
    // Switch on the kind of n1, arbitrarily, and go deeper.
    switch (n1->get_kind()) {
        case (tastr::ast::Node::Kind::ClassType): {
            auto n1__class = (tastr::ast::ClassTypeNodePtr) n1;
            auto n2__class = (tastr::ast::ClassTypeNodePtr) n2;
            return are_nodes_equal(&n1__class->get_parameters(), &n2__class->get_parameters());
        }
        case (tastr::ast::Node::Kind::CommaSeparator): {
            auto n1__commaSeparator = (tastr::ast::CommaSeparatorNodePtr) n1;
            auto n2__commaSeparator = (tastr::ast::CommaSeparatorNodePtr) n2;
            return are_nodes_equal(&n1__commaSeparator->get_first_node(), &n2__commaSeparator->get_first_node()) &&
                are_nodes_equal(&n1__commaSeparator->get_second_node(), &n2__commaSeparator->get_second_node());
        }
        case (tastr::ast::Node::Kind::FunctionType): {
            auto n1__functionType = (tastr::ast::FunctionTypeNodePtr) n1;
            auto n2__functionType = (tastr::ast::FunctionTypeNodePtr) n2;

            // Check that all parameters are equal.
            auto n1__param = (tastr::ast::ParameterNodePtr) &n1__functionType->get_parameter();
            auto n2__param = (tastr::ast::ParameterNodePtr) &n2__functionType->get_parameter();

            // If there aren't the same # of parameters, they can't be equal.
            if (n1__param->get_parameter_count() != n2__param->get_parameter_count()) {
                return false;
            }

            // Check that all of the parameters are pairwise equal.
            bool allGood = true;
            for (int i = 0; i < n1__param->get_parameter_count(); ++i) {
                allGood &= are_nodes_equal(&n1__param->at(i), &n2__param->at(i));
            }

            // Check that the return is also equal.
            allGood &= are_nodes_equal(&n1__functionType->get_return_type(), &n2__functionType->get_return_type());

            return allGood;
        }
        case (tastr::ast::Node::Kind::GroupType): {
            auto n1__group = (tastr::ast::GroupTypeNodePtr) n1;
            auto n2__group = (tastr::ast::GroupTypeNodePtr) n2;

            return are_nodes_equal(&n1__group->get_inner_type(), &n2__group->get_inner_type());
        }
        case (tastr::ast::Node::Kind::Identifier): {
            auto n1__identifier = (tastr::ast::IdentifierNodePtr) n1;
            auto n2__identifier = (tastr::ast::IdentifierNodePtr) n2;
            return n1__identifier->get_name() == n2__identifier->get_name();
        }
        case (tastr::ast::Node::Kind::IntersectionType): {
            auto n1__intersection = (tastr::ast::IntersectionTypeNodePtr) n1;
            auto n2__intersection = (tastr::ast::IntersectionTypeNodePtr) n2;
            return are_nodes_equal(&n1__intersection->get_first_type(), &n2__intersection->get_first_type()) &&
                are_nodes_equal(&n1__intersection->get_second_type(), &n2__intersection->get_second_type());
        }
        case (tastr::ast::Node::Kind::ListType): {
            auto n1__list = (tastr::ast::ListTypeNodePtr) n1;
            auto n2__list = (tastr::ast::ListTypeNodePtr) n2;
            return are_nodes_equal(&n1__list->get_parameters(), &n2__list->get_parameters());
        }
        case (tastr::ast::Node::Kind::NAScalarType): {
            auto n1__naScalar = (tastr::ast::NAScalarTypeNodePtr) n1;
            auto n2__naScalar = (tastr::ast::NAScalarTypeNodePtr) n2;
            return are_nodes_equal(&n1__naScalar->get_a_scalar_type(), &n2__naScalar->get_a_scalar_type());
        }
        case (tastr::ast::Node::Kind::NullableType): {
            auto n1__nullable = (tastr::ast::NullableTypeNodePtr) n1;
            auto n2__nullable = (tastr::ast::NullableTypeNodePtr) n2;
            return are_nodes_equal(&n1__nullable->get_inner_type(), &n2__nullable->get_inner_type());
        }
        case (tastr::ast::Node::Kind::Parameter): {
            auto n1__parameter = (tastr::ast::ParameterNodePtr) n1;
            auto n2__parameter = (tastr::ast::ParameterNodePtr) n2;
            return are_nodes_equal(&n1__parameter->get_elements(), &n2__parameter->get_elements());
        }
        // This doesn't really occur right now, not for the use case we have in mind. 
        // case (tastr::ast::Node::Kind::TopLevel):
        // Neither does this.
        // case (tastr::ast::Node::Kind::TypeDeclaration):
        case (tastr::ast::Node::Kind::UnionType): {
            auto n1__union = (tastr::ast::UnionTypeNodePtr) n1;
            auto n2__union = (tastr::ast::UnionTypeNodePtr) n2;
            return are_nodes_equal(&n1__union->get_first_type(), &n2__union->get_first_type()) &&
                are_nodes_equal(&n1__union->get_second_type(), &n2__union->get_second_type());
        }
        case (tastr::ast::Node::Kind::VectorType): {
            auto n1__vector = (tastr::ast::VectorTypeNodePtr) n1;
            auto n2__vector = (tastr::ast::VectorTypeNodePtr) n2;
            return are_nodes_equal(&n1__vector->get_scalar_type(), &n2__vector->get_scalar_type());
        }
        default: 
            return true;
    }
}

std::vector<const tastr::ast::TypeNode*> remove_duplicate_nodes(std::vector<const tastr::ast::TypeNode*> nodes) {
    std::vector<const tastr::ast::TypeNode*> uniqueNodes;
    for (int i = 0; i < nodes.size(); ++i) {
        auto thisNode = nodes[i];
        bool isUnique = true;
        for (int j = i + 1; j < nodes.size(); ++j) {
            if (are_nodes_equal(thisNode, nodes[j])) {
                isUnique = false;
                break;
            }
        }
        if (isUnique) uniqueNodes.push_back(thisNode);
    }
    return uniqueNodes;
}

std::vector<const tastr::ast::TypeNode*> collect_union_elements(const tastr::ast::Node* union__node) {
    auto iter = &((tastr::ast::UnionTypeNodePtr) union__node)->get_first_type();
    std::vector<const tastr::ast::TypeNode*> unionElts;
    unionElts.push_back(&((tastr::ast::UnionTypeNodePtr) union__node)->get_second_type());
    size_t theSize = 2;
    while (iter->get_kind() == tastr::ast::Node::Kind::UnionType) {
        unionElts.push_back(&((tastr::ast::UnionTypeNodePtr) iter)->get_second_type());
        iter = &((tastr::ast::UnionTypeNodePtr) iter)->get_first_type();
        theSize++;
    }

    unionElts.push_back((tastr::ast::TypeNode*) iter);

    return unionElts;
}

// Function that takes two sigs, and returns how many parameters they differ in.
SEXP r_eq_distance(SEXP sig1, SEXP sig2) {
    auto node1 = (tastr::ast::FunctionTypeNode*) R_ExternalPtrAddr(sig1);
    auto node2 = (tastr::ast::FunctionTypeNode*) R_ExternalPtrAddr(sig2);

    auto node1__params = (tastr::ast::ParameterNodePtr) &node1->get_parameter();
    auto node2__params = (tastr::ast::ParameterNodePtr) &node2->get_parameter();

    // Are they the same size?
    if (node1__params->get_parameter_count() != node2__params->get_parameter_count()) {
        return ScalarInteger(-1);
    }

    // They are the same size.
    int numEqual = 0;
    for (int i = 0; i < node1__params->get_parameter_count(); ++i) {
        if (are_nodes_equal(&node1__params->at(i), &node2__params->at(i))) {
            numEqual++;
        } 
    }

    if (are_nodes_equal(&node1->get_return_type(), &node2->get_return_type())) {
        numEqual++;
    }

    return ScalarInteger(node1__params->get_parameter_count() + 1 - numEqual);
}

tastr::ast::UnionTypeNodePtr new_union_from_list_of_elements(std::vector<const tastr::ast::TypeNode*> elts) {
    // Note: elts.size() should be > 1 (otherwise, what are you doing creating a union of 1 elt?)
    auto theUnion = new tastr::ast::UnionTypeNode((new tastr::ast::OperatorNode("|"))->clone(),
        elts[0]->clone(), elts[1]->clone());

    // If there are more elements, add them.
    for (int i = 2; i < elts.size(); ++i) {
        theUnion = new tastr::ast::UnionTypeNode((new tastr::ast::OperatorNode("|"))->clone(),
        theUnion->clone(), elts[i]->clone());
    } 

    return theUnion;
}

std::vector<const tastr::ast::TypeNode*> minimize_list_of_types_with_subtyping(std::vector<const tastr::ast::TypeNode*> unionElts) {
    // Create vector of booleans to track which types are subsumed.
    std::vector<bool> isSubsumed(unionElts.size(), false);

    // Can optimize this to halve the iterations (by setting i and j).
    for (int i = 0; i < unionElts.size(); ++i) {
        for (int j = 0; j < unionElts.size(); ++j) {
            // is unionElts[i] <: unionElts[j]?
            isSubsumed[i] = R_TrueValue == r_is_subtype_inner(unionElts[i], unionElts[j]);
            // If we've determined that unionElts[i] is subsumed, then we don't need to do anything else for it.
            if (isSubsumed[i]) {
                break;
            }
        }
    }

    // Make a new type with the non-subsumed types.
    std::vector<const tastr::ast::TypeNode*> leftoverTypes;
    for (int i = 0; i < isSubsumed.size(); ++i) {
        if (!isSubsumed[i]) {
            leftoverTypes.push_back(unionElts[i]);
        }
    }

    return leftoverTypes;
}

// Returns a minimized union node.
const tastr::ast::Node* minimize_union_type_with_subtyping(const tastr::ast::Node* union__node) {
    // Make a vector with all of the union elements.
    auto unionElts = collect_union_elements(union__node);
    unionElts = remove_duplicate_nodes(unionElts);

    std::vector<const tastr::ast::TypeNode*> leftoverTypes = minimize_list_of_types_with_subtyping(unionElts);

    if (leftoverTypes.size() == 1) {
        // Just return it.
        return leftoverTypes[0];
    } else {
        // Make a new one.
        return new_union_from_list_of_elements(leftoverTypes);
    }
}

SEXP r_is_subtype(SEXP type1, SEXP type2) {
    auto node1 = (tastr::ast::Node*) R_ExternalPtrAddr(type1);
    auto node2 = (tastr::ast::Node*) R_ExternalPtrAddr(type2);

    return r_is_subtype_inner(node1, node2);
}

// Function which creates a new AST by simplifying the unions in the given AST.
// Is there a better way to do unique_ptr casting? get and clone is kind of cumbersome.
tastr::ast::NodeUPtr simplify(const tastr::ast::Node* input) {
    switch (input->get_kind()) {
        case tastr::ast::Node::Kind::UnionType: {
            auto inputAsUnion = ((tastr::ast::UnionTypeNodePtr) input);

            // Get all union elements.
            auto unionElts = collect_union_elements(inputAsUnion);

            // Simplify all of the union elements.
            std::vector<tastr::ast::TypeNodeUPtr> simplifiedUnionElts;
            for (int i = 0; i < unionElts.size(); ++i) {
                simplifiedUnionElts.push_back(((tastr::ast::TypeNodePtr) simplify(unionElts[i]).get())->clone());
            }

            // Make a new union.
            auto newUnion = new tastr::ast::UnionTypeNode((new tastr::ast::OperatorNode("|"))->clone(),
                simplifiedUnionElts[0]->clone(), simplifiedUnionElts[1]->clone());

            for (int i = 2; i < simplifiedUnionElts.size(); ++i) {
                newUnion = new tastr::ast::UnionTypeNode((new tastr::ast::OperatorNode("|"))->clone(),
                    newUnion->clone(), simplifiedUnionElts[i]->clone());
            } 

            // Minimize the new union.
            return minimize_union_type_with_subtyping(newUnion)->clone();
        }
        case tastr::ast::Node::Kind::ListType: {
            auto inputAsList = ((tastr::ast::ListTypeNodePtr) input);
            return (new tastr::ast::ListTypeNode(
                inputAsList->get_keyword().clone(),
                ((tastr::ast::ParameterNodePtr) simplify(&inputAsList->get_parameters()).get())->clone()
            ))->clone();
        }
        case tastr::ast::Node::Kind::Parameter: {
            auto inputAsParameter = ((tastr::ast::ParameterNodePtr) input);
            return (new tastr::ast::ParameterNode(
                // (new tastr::ast::OperatorNode("<"))->clone(),
                // (new tastr::ast::OperatorNode(">"))->clone(),
                inputAsParameter->get_opening_bracket().clone(),
                inputAsParameter->get_closing_bracket().clone(),
                simplify(&inputAsParameter->get_elements())
            ))->clone();
        }
        case tastr::ast::Node::Kind::CommaSeparator: {
            auto inputAsCommaSeparator = ((tastr::ast::CommaSeparatorNodePtr) input);
            return (new tastr::ast::CommaSeparatorNode(
                inputAsCommaSeparator->get_separator().clone(),
                // (new tastr::ast::SeparatorNode(","))->clone(),
                simplify(&inputAsCommaSeparator->get_first_node()),
                simplify(&inputAsCommaSeparator->get_second_node())
            ))->clone();
        }
        case tastr::ast::Node::Kind::FunctionType: {
            auto inputAsFunctionType = ((tastr::ast::FunctionTypeNodePtr) input);
            return (new tastr::ast::FunctionTypeNode(
                inputAsFunctionType->get_operator().clone(),
                // (new tastr::ast::OperatorNode("->"))->clone(),
                simplify(&inputAsFunctionType->get_parameter()),
                ((tastr::ast::TypeNodePtr) simplify(&inputAsFunctionType->get_return_type()).get())->clone()
            ))->clone();
        }
        case tastr::ast::Node::Kind::GroupType: {
            auto inputAsGroup = ((tastr::ast::GroupTypeNodePtr) input);
            return (new tastr::ast::GroupTypeNode(
                inputAsGroup->get_opening_bracket().clone(),
                inputAsGroup->get_closing_bracket().clone(),
                // (new tastr::ast::OperatorNode("("))->clone(),
                // (new tastr::ast::OperatorNode(")"))->clone(),
                ((tastr::ast::TypeNodePtr) simplify(&inputAsGroup->get_inner_type()).get())->clone()
            ))->clone();
        }
        default: {
            return input->clone();
        }
    }
}

SEXP r_minimize_signature(SEXP sig) {
    auto sigNode = (tastr::ast::Node*) R_ExternalPtrAddr(sig);

    // Minimize the unions in the node.
    auto retMe = simplify(sigNode);

    return node2extptr(retMe.release());
}

std::vector<const tastr::ast::TypeNode*> collect_and_combine_two_types(tastr::ast::TypeNodePtr n1, tastr::ast::TypeNodePtr n2) {
    // Make two lists.
        std::vector<const tastr::ast::TypeNode*> node1__params__list;
        if (n1->get_kind() == tastr::ast::Node::Kind::UnionType) {
            // Collect.
            node1__params__list = collect_union_elements(n1);
        } else {
            // Just put it in a list by itself.
            node1__params__list.push_back(n1);
        }

        std::vector<const tastr::ast::TypeNode*> node2__params__list;
        if (n2->get_kind() == tastr::ast::Node::Kind::UnionType) {
            // Collect.
            node2__params__list = collect_union_elements(n2);
        } else {
            // Just put it in a list by itself.
            node2__params__list.push_back(n2);
        }

        // Combine the lists.
        std::vector<const tastr::ast::TypeNode*> all_types;
        all_types.reserve(node1__params__list.size() + node2__params__list.size());
        all_types.insert(all_types.end(), node1__params__list.begin(), node1__params__list.end());
        all_types.insert(all_types.end(), node2__params__list.begin(), node2__params__list.end());

        return all_types;
}

tastr::ast::Node* create_node_from_list(std::vector<const tastr::ast::TypeNode*> types) {
    if (types.size() == 1) {
        return (tastr::ast::Node*) types[0];
    } else {
        return new_union_from_list_of_elements(types);
    }
}

// Function that tries to combine two sigs.
// Returns false if the new union(s) can't be simplified.
// If the return + all params managed to get simplified, returns the combined signature.
SEXP r_combine_sigs(SEXP sig1, SEXP sig2) {
    auto node1 = (tastr::ast::FunctionTypeNode*) R_ExternalPtrAddr(sig1);
    auto node2 = (tastr::ast::FunctionTypeNode*) R_ExternalPtrAddr(sig2);

    auto node1__params = (tastr::ast::ParameterNodePtr) &node1->get_parameter();
    auto node2__params = (tastr::ast::ParameterNodePtr) &node2->get_parameter();

    // Are they the same size?
    if (node1__params->get_parameter_count() != node2__params->get_parameter_count()) {
        return R_FalseValue;
    }

    std::vector<std::vector<const tastr::ast::TypeNode*>> param_type_lists;

    // They are the same size.
    // Deal with parameters.
    for (int i = 0; i < node1__params->get_parameter_count(); ++i) {
        // Combine the two, and minimize.
        // Make two lists, put them together, minimize them.
        // If length(combined) == length(minimized), then this was a waste.
        // Otherwise, we did good. 
        std::vector<const tastr::ast::TypeNode*> all_types = collect_and_combine_two_types((tastr::ast::TypeNodePtr) &node1__params->at(i), (tastr::ast::TypeNodePtr) &node2__params->at(i));

        // Minimize combined list.
        auto deduped_all_types = remove_duplicate_nodes(all_types);
        auto minimized_all_types = minimize_list_of_types_with_subtyping(deduped_all_types);

        // Note: What about 
        if (minimized_all_types.size() == all_types.size()) {
            // We didn't make progress, bail.
            return R_FalseValue;
        } else {
            param_type_lists.push_back(minimized_all_types);
        }
    }

    // Deal with return.
    std::vector<const tastr::ast::TypeNode*> all_returns = collect_and_combine_two_types((tastr::ast::TypeNodePtr) &node1->get_return_type(), (tastr::ast::TypeNodePtr) &node2->get_return_type());
    auto minimized_return_types = minimize_list_of_types_with_subtyping(remove_duplicate_nodes(all_returns));

    if (minimized_return_types.size() == all_returns.size()) {
        // We didn't make progress, bail.
        return R_FalseValue;
    }

    // If we've made it here, the new signature is good to go.
    
    tastr::ast::NodePtr parameter;
    if (node1__params->get_parameter_count() == 0) {
        // TODO Nothing to do? Make sure this works.
        parameter = (tastr::ast::NodePtr) new tastr::ast::EmptyNode();
    } else if (node1__params->get_parameter_count() == 1) {
        // Just the node, thanks.
        parameter = create_node_from_list(param_type_lists[0]);
        // if (param_type_lists[0].size() == 1) {
        //     // Actually just the node, thanks.
        //     parameter = (tastr::ast::NodePtr) param_type_lists[0].at(0);
        // } else {
        //     // Make it a union.
        //     parameter = (tastr::ast::NodePtr) new_union_from_list_of_elements(param_type_lists[0]);
        // }
    } else {
        // Need a (big) CommaSeparatorNode.
        auto fst = create_node_from_list(param_type_lists[0])->clone();
        auto snd = create_node_from_list(param_type_lists[1])->clone();
        auto theCommaSeparatorNode = new tastr::ast::CommaSeparatorNode(
            (new tastr::ast::SeparatorNode(","))->clone(),
            create_node_from_list(param_type_lists[0])->clone(), 
            create_node_from_list(param_type_lists[1])->clone()
        );

        // If there are more elements, add them.
        for (int i = 2; i < param_type_lists.size(); ++i) {
            theCommaSeparatorNode = new tastr::ast::CommaSeparatorNode(
                (new tastr::ast::SeparatorNode(","))->clone(),
                theCommaSeparatorNode->clone(), 
                create_node_from_list(param_type_lists[i])->clone()
            );
        } 

        parameter = theCommaSeparatorNode;
    }

    tastr::ast::TypeNodePtr return_type;
    if (minimized_return_types.size() == 1) {
        return_type = (tastr::ast::TypeNodePtr) minimized_return_types[0];
    } else {
        return_type = (tastr::ast::TypeNodePtr) new_union_from_list_of_elements(minimized_return_types);
    }

    return node2extptr(new tastr::ast::FunctionTypeNode(
        // op,
        node1->get_operator().clone(),
        parameter->clone(),
        return_type->clone()
    ));
}

SEXP r_is_function_type(SEXP type) {
    auto node = (tastr::ast::Node*) R_ExternalPtrAddr(type);
    return Rf_ScalarLogical(node->is_function_type_node());
}

SEXP r_is_class_type(SEXP type) {
    auto node = (tastr::ast::Node*) R_ExternalPtrAddr(type);
    return Rf_ScalarLogical(node->is_class_type_node());
}

SEXP r_get_classes(SEXP type) {
    auto node = (tastr::ast::Node*) R_ExternalPtrAddr(type);
    SEXP ret;
    if (!node->is_class_type_node()) {
        ret = PROTECT(Rf_allocVector(STRSXP, 0));
    } else {
        auto class_node = (tastr::ast::ClassTypeNode*) node;
        auto params = class_node->get_parameters();
        ret = PROTECT(Rf_allocVector(STRSXP, params.get_parameter_count()));

        for (int i = 0; i < params.get_parameter_count(); i++) {
            auto param_node = tastr::ast::as<tastr::ast::IdentifierNode>(params.at(i));
            auto name = param_node.get_name();                
            SET_STRING_ELT(ret, i, Rf_mkChar(name.c_str()));
        }
    }

    UNPROTECT(1);
    return ret;
}

SEXP r_get_parameter_type(SEXP type, SEXP param) {
    auto node = (tastr::ast::Node*) R_ExternalPtrAddr(type);

    if (!node->is_function_type_node()) {
        Rf_error("Not a function type");
        return R_NilValue;
    }

    auto idx = INTEGER(param)[0];
    auto fun_node = (tastr::ast::FunctionTypeNode*) node;

    tastr::ast::Node* param_node;

    if (idx == 0) {
        param_node = get_function_return_type(fun_node).clone().release();
    } else {
        param_node = get_function_parameter_type(fun_node, idx - 1).clone().release();
    }

    return node2extptr(param_node);
}

SEXP type_to_sexp_string(SEXP type) {
    auto node = (tastr::ast::Node*) R_ExternalPtrAddr(type);
    auto str = type_to_string(*node);
    return Rf_mkString(str.c_str());
}
