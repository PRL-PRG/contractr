#include "type_declaration_cache.hpp"

#include "utilities.hpp"

#undef length
#include <filesystem>
#include <tastr/parser/parser.hpp>
#include <unordered_map>

namespace fs = std::filesystem;

using package_name_t = std::string;
using function_name_t = std::string;

using package_type_declaration_t =
    std::unordered_map<function_name_t, tastr::ast::TypeNodeUPtr>;

static std::unordered_map<package_name_t, package_type_declaration_t>
    type_declaration_cache;

fs::path type_declaration_directory = "";

void initialize_type_declaration_cache() {
    SEXP path = PROTECT(mkString("typedecl"));
    type_declaration_directory = fs::path(CHAR(asChar(system_file(path))));
    UNPROTECT(1);

    if (!fs::is_directory(type_declaration_directory)) {
        errorcall(R_NilValue,
                  "type declaration directory '%s' does not exist",
                  type_declaration_directory.c_str());
    }
}

SEXP clear_type_declaration_cache() {
    type_declaration_cache.clear();
    return R_NilValue;
}

SEXP import_type_declarations(SEXP pkg_name) {
    const std::string package_name = CHAR(asChar(pkg_name));

    fs::path package_typedecl_filepath =
        type_declaration_directory / fs::path(package_name);

    /* return empty vector if types are not present  */
    if (!fs::is_regular_file(package_typedecl_filepath)) {
        return allocVector(STRSXP, 0);
    }

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

    package_type_declaration_t package_map;

    SEXP function_names =
        PROTECT(allocVector(STRSXP, result.get_top_level_node()->size()));

    int index = 0;
    for (const std::unique_ptr<tastr::ast::TypeDeclarationNode>& decl:
         result.get_top_level_node()->get_type_declarations()) {
        std::string name(decl->get_identifier().get_name());
        package_map.insert(std::make_pair(name, decl->get_type().clone()));
        SET_STRING_ELT(function_names, index, mkChar(name.c_str()));
    }

    type_declaration_cache.insert({package_name, std::move(package_map)});

    UNPROTECT(1);

    return function_names;
}

SEXP get_typed_package_names() {
    SEXP result = PROTECT(allocVector(STRSXP, type_declaration_cache.size()));
    int index = 0;
    for (auto iter = type_declaration_cache.begin();
         iter != type_declaration_cache.end();
         ++iter, ++index) {
        const std::string& package_name = iter->first;
        SET_STRING_ELT(result, index, mkChar(package_name.c_str()));
    }

    UNPROTECT(1);

    return result;
}

SEXP get_typed_function_names(SEXP pkg_name) {
    if (is_package_typed(pkg_name) != R_TrueValue) {
        return allocVector(STRSXP, 0);
    }

    const std::string package_name = CHAR(asChar(pkg_name));
    auto package_iter = type_declaration_cache.find(package_name);

    const package_type_declaration_t& package_map = package_iter->second;

    SEXP result = PROTECT(allocVector(STRSXP, package_map.size()));
    int index = 0;

    for (auto iter = package_map.begin(); iter != package_map.end();
         ++iter, ++index) {
        const std::string& function_name = iter->first;
        SET_STRING_ELT(result, index, mkChar(function_name.c_str()));
    }

    UNPROTECT(1);

    return result;
}

SEXP is_package_typed(SEXP pkg_name) {
    const std::string package_name = CHAR(asChar(pkg_name));
    auto package_iter = type_declaration_cache.find(package_name);
    if (package_iter == type_declaration_cache.end()) {
        return R_FalseValue;
    } else {
        return R_TrueValue;
    }
}

SEXP is_function_typed(SEXP pkg_name, SEXP fun_name) {
    const std::string package_name = CHAR(asChar(pkg_name));
    const std::string function_name = CHAR(asChar(fun_name));

    auto package_iter = type_declaration_cache.find(package_name);
    if (package_iter == type_declaration_cache.end()) {
        return R_FalseValue;
    } else {
        const package_type_declaration_t& package_map = package_iter->second;
        auto iter = package_map.find(function_name);
        if (iter == package_map.end()) {
            return R_FalseValue;
        } else {
            return R_TrueValue;
        }
    }
}

SEXP set_type_declaration(SEXP pkg_name, SEXP fun_name, SEXP type_decl) {
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

    const tastr::ast::TypeDeclarationNode& decl =
        result.get_top_level_node()->at(0);

    auto package_iter = type_declaration_cache.find(package_name);

    if (package_iter == type_declaration_cache.end()) {
        package_type_declaration_t package_map;
        package_map.insert(
            std::make_pair(function_name, decl.get_type().clone()));
        type_declaration_cache.insert({package_name, std::move(package_map)});
    }

    else {
        package_type_declaration_t& package_map = package_iter->second;
        package_map.insert(
            std::make_pair(function_name, decl.get_type().clone()));
    }

    return R_NilValue;
}

SEXP remove_type_declaration(SEXP pkg_name, SEXP fun_name) {
    const std::string package_name = CHAR(asChar(pkg_name));
    const std::string function_name = CHAR(asChar(fun_name));

    auto package_iter = type_declaration_cache.find(package_name);

    if (package_iter == type_declaration_cache.end()) {
        return R_FalseValue;
    }

    else {
        package_type_declaration_t& package_map = package_iter->second;
        auto iter = package_map.find(function_name);

        if (iter == package_map.end()) {
            return R_FalseValue;
        }

        else {
            package_map.erase(iter);
            return R_TrueValue;
        }
    }
}

void show_function_type_declaration_(const std::string& package_name,
                                     const std::string& function_name,
                                     const tastr::ast::TypeNodeUPtr& type) {
    tastr::ast::TypeDeclarationNode node(
        /* FIXME: hacky space */
        std::move(wrap(new tastr::ast::KeywordNode("type "))),
        std::move(wrap(new tastr::ast::IdentifierNode(function_name))),
        type->clone(),
        std::move(wrap(new tastr::ast::TerminatorNode(";"))));

    bool style = true;
    tastr::parser::unparse_stdout(node, false, style);
    std::cout << std::endl;
}

void show_package_type_declarations_(
    const std::string& package_name,
    const package_type_declaration_t& package) {
    std::cout << std::string(80, '#') << std::endl;
    std::cout << "## " << package_name << std::endl;
    std::cout << std::string(80, '#') << std::endl;
    for (auto function_iter = package.cbegin(); function_iter != package.cend();
         ++function_iter) {
        show_function_type_declaration_(
            package_name, function_iter->first, function_iter->second);
    }
    std::cout << std::endl;
}

SEXP show_function_type_declaration(SEXP pkg_name, SEXP fun_name) {
    const std::string package_name = CHAR(asChar(pkg_name));
    const std::string function_name = CHAR(asChar(fun_name));

    auto package_iter = type_declaration_cache.find(package_name);
    if (package_iter != type_declaration_cache.end()) {
        const package_type_declaration_t& package = package_iter->second;
        auto function_iter = package.find(function_name);
        if (function_iter != package.end()) {
            show_function_type_declaration_(
                package_name, function_iter->first, function_iter->second);
        }
    }
    return R_NilValue;
}

SEXP show_package_type_declarations(SEXP pkg_name) {
    const std::string package_name = CHAR(asChar(pkg_name));
    auto iter = type_declaration_cache.find(package_name);
    if (iter != type_declaration_cache.end()) {
        show_package_type_declarations_(package_name, iter->second);
    }
    return R_NilValue;
}

SEXP show_type_declarations() {
    for (auto package_iter = type_declaration_cache.cbegin();
         package_iter != type_declaration_cache.cend();
         ++package_iter) {
        show_package_type_declarations_(package_iter->first,
                                        package_iter->second);
    }
    return R_NilValue;
}

const tastr::ast::Node*
get_function_parameter_type(const std::string& package_name,
                            const std::string& function_name,
                            int formal_parameter_position) {
    const tastr::ast::FunctionTypeNode* function_type =
        get_function_type(package_name, function_name);

    if (function_type == nullptr) {
        return nullptr;
    }

    const tastr::ast::ParameterNode& parameter_node =
        function_type->get_parameter();

    return &parameter_node.at(formal_parameter_position);
}

const tastr::ast::Node*
get_function_return_type(const std::string& package_name,
                         const std::string& function_name) {
    const tastr::ast::FunctionTypeNode* function_type =
        get_function_type(package_name, function_name);

    if (function_type == nullptr) {
        return nullptr;
    }

    return &function_type->get_return_type();
}

const tastr::ast::FunctionTypeNode*
get_function_type(const std::string& package_name,
                  const std::string& function_name) {
    auto package_iter = type_declaration_cache.find(package_name);

    if (package_iter == type_declaration_cache.end()) {
        return nullptr;
    }

    const package_type_declaration_t& package_map = package_iter->second;

    auto function_iter = package_map.find(function_name);

    if (function_iter == package_map.end()) {
        return nullptr;
    }

    tastr::ast::TypeNode* node = function_iter->second.get();

    if (node->is_function_type_node()) {
        return tastr::ast::as<tastr::ast::FunctionTypeNode>(node);
    } else {
        return nullptr;
    }
}

std::string type_to_string(const tastr::ast::Node& node) {
    return tastr::parser::to_string(node);
}
