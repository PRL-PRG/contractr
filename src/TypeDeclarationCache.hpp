#ifndef CONTRACTR_TYPE_DECLARATION_CACHE_H
#define CONTRACTR_TYPE_DECLARATION_CACHE_H

#include "logger.hpp"

#undef length
#include <filesystem>
#include <memory>
#include <string>
#include <tastr/ast/ast.hpp>
#include <tastr/parser/parser.hpp>
#include <unordered_map>

namespace fs = std::filesystem;

class TypeDeclarationCache {
  public:
    TypeDeclarationCache() {
    }

    ~TypeDeclarationCache() = default;

    static fs::path get_type_declaration_dirpath() {
        if (fs::is_directory(type_declaration_dirpath_)) {
            return fs::canonical(type_declaration_dirpath_);
        } else {
            return fs::canonical(type_declaration_dirpath_);
        }
    }

    static fs::path
    get_package_type_declaration_filepath(const std::string& package_name) {
        return get_type_declaration_dirpath() / package_name;
    }

    static bool package_is_typed(const std::string& package_name) {
        return fs::is_regular_file(get_package_type_declaration_filepath(package_name));
    }

    static bool function_is_typed(const std::string& package_name,
                                  const std::string& function_name) {
        return get_function_type(package_name, function_name) != nullptr;
    }

    static const tastr::ast::Node*
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

    static const tastr::ast::Node*
    get_function_return_type(const std::string& package_name,
                             const std::string& function_name) {
        const tastr::ast::FunctionTypeNode* function_type =
            get_function_type(package_name, function_name);

        if (function_type == nullptr) {
            return nullptr;
        }

        return &function_type->get_return_type();
    }

    static const tastr::ast::FunctionTypeNode*
    get_function_type(const std::string& package_name,
                      const std::string& function_name) {
        const fs::path package_path =
            get_package_type_declaration_filepath(package_name);

        if (!fs::is_regular_file(package_path)) {
            log_warn("'%s' is not a file or does not exist",
                     package_path.c_str());
            return nullptr;
        }

        auto package = packages_.find(package_name);

        if (package == packages_.end()) {
            package = packages_
                          .insert({package_name,
                                   import_type_declarations_(package_path)})
                          .first;
        }

        auto type_iter = package->second.find(function_name);
        if (type_iter == package->second.end()) {
            return nullptr;
        }

        tastr::ast::TypeNode* node = type_iter->second.get();

        if (node->is_function_type_node()) {
            return tastr::ast::as<tastr::ast::FunctionTypeNode>(node);
        } else {
            return nullptr;
        }
    }

  private:
    static std::unordered_map<std::string, tastr::ast::TypeNodeUPtr>
    import_type_declarations_(
        const fs::path& package_type_declaration_filepath) {
        tastr::parser::ParseResult result(
            tastr::parser::parse_file(package_type_declaration_filepath));

        std::unordered_map<std::string, tastr::ast::TypeNodeUPtr> package_map;

        for (const std::unique_ptr<tastr::ast::TypeDeclarationNode>& decl:
             result.get_top_level_node()->get_type_declarations()) {
            std::string name(decl->get_identifier().get_name());
            package_map.insert(std::make_pair(name, decl->get_type().clone()));
        }
        return package_map;
    }

    static std::unordered_map<
        std::string,
        std::unordered_map<std::string, tastr::ast::TypeNodeUPtr>>
        packages_;

    static const fs::path type_declaration_dirpath_;
};

#endif /* CONTRACTR_TYPE_DECLARATION_CACHE_H */
