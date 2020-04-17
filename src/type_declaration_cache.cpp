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
        log_warn("type declaration directory '%s' does not exist",
                 type_declaration_directory.c_str());
    }
}

SEXP import_type_declarations(SEXP pkg_name) {
    const std::string& package_name = CHAR(asChar(pkg_name));

    fs::path package_typedecl_filepath =
        type_declaration_directory / fs::path(package_name);

    if (!fs::is_regular_file(package_typedecl_filepath)) {
        log_warn("'%s' is not a file or does not exist",
                 package_typedecl_filepath.c_str());
        return R_FalseValue;
    }

    tastr::parser::ParseResult result(
        tastr::parser::parse_file(package_typedecl_filepath));

    if (!result) {
        log_warn("%s:%s :: %s\n",
                 package_typedecl_filepath.c_str(),
                 to_string(result.get_error_location()).c_str(),
                 result.get_error_message().c_str());
        return R_FalseValue;
    }

    package_type_declaration_t package_map;

    for (const std::unique_ptr<tastr::ast::TypeDeclarationNode>& decl:
         result.get_top_level_node()->get_type_declarations()) {
        std::string name(decl->get_identifier().get_name());
        package_map.insert(std::make_pair(name, decl->get_type().clone()));
    }

    type_declaration_cache.insert({package_name, std::move(package_map)});

    return R_TrueValue;
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

    const std::string& package_name = CHAR(asChar(pkg_name));
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
    const std::string& package_name = CHAR(asChar(pkg_name));
    auto package_iter = type_declaration_cache.find(package_name);
    if (package_iter == type_declaration_cache.end()) {
        return R_FalseValue;
    } else {
        return R_TrueValue;
    }
}

SEXP is_function_typed(SEXP pkg_name, SEXP fun_name) {
    const std::string& package_name = CHAR(asChar(pkg_name));
    const std::string& function_name = CHAR(asChar(pkg_name));

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
