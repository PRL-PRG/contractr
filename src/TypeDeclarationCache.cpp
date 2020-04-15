#include "TypeDeclarationCache.hpp"

std::unordered_map<std::string,
                   std::unordered_map<std::string, tastr::ast::TypeNodeUPtr>>
    TypeDeclarationCache::packages_;

const fs::path TypeDeclarationCache::type_declaration_dirpath_ = "/tmp/typecache/";
