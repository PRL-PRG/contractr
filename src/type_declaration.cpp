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

SEXP r_is_function_type(SEXP type) {
    auto node = (tastr::ast::Node*) R_ExternalPtrAddr(type);
    return Rf_ScalarLogical(node->is_function_type_node());
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
