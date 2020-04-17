#ifndef CONTRACTR_TYPECHECKER_HPP
#define CONTRACTR_TYPECHECKER_HPP

#include "logger.hpp"

#include <cassert>
#include <functional>
#include <tastr/visitor/visitor.hpp>

class TypeChecker final: public tastr::visitor::ConstNodeVisitor {
  private:
    using seq_index_t = int;

    const std::string& package_name_;
    const std::string& function_name_;
    const std::string& parameter_name_;
    int formal_parameter_position_;
    bool result_;

    bool is_dot_dot_dot_parameter_() {
        return parameter_name_ == "...";
    }

  public:
    TypeChecker(const std::string& package_name,
                const std::string& function_name,
                const std::string& parameter_name,
                int formal_parameter_position)
        : ConstNodeVisitor()
        , package_name_(package_name)
        , function_name_(function_name)
        , parameter_name_(parameter_name)
        , formal_parameter_position_(formal_parameter_position)
        , result_(false) {
    }

    bool typecheck(SEXP value, const tastr::ast::Node& node) {
        if (is_dot_dot_dot_parameter_()) {
            push_result_(node.is_vararg_type_node());
        } else {
            na_ = false;
            vector_ = false;
            push_value_(value);
            node.accept(*this);
        }
        return pop_result_();
    }

    void visit(const tastr::ast::IdentifierNode& node) override final {
        SEXP value = pop_value_();
        std::string name = node.get_name();
        push_result_(CHAR(value) == name);
    }

    void visit(const tastr::ast::VectorTypeNode& node) override final {
        SEXP value = pop_value_();
        bool result = is_vector_type_(value);
        if (!result) {
            push_result_(result);
        } else {
            push_value_(value);
            set_vector_();
            node.get_scalar_type().accept(*this);
            unset_vector_();
        }
    }

    void visit(const tastr::ast::NAScalarTypeNode& node) override final {
        /* setting this is important because we want to make sure that there
         * is no na value if na annotation is not present */
        set_na_();
        node.get_a_scalar_type().accept(*this);
        unset_na_();
    }

    void
    visit(const tastr::ast::CharacterAScalarTypeNode& node) override final {
        SEXP value = pop_value_();
        satisfies_vector_or_scalar_(
            value, STRSXP, [](SEXP vector, int index) -> bool {
                return STRING_ELT(vector, index) == NA_STRING;
            });
    }

    void visit(const tastr::ast::ComplexAScalarTypeNode& node) override final {
        SEXP value = pop_value_();
        satisfies_vector_or_scalar_(
            value, CPLXSXP, [](SEXP vector, int index) -> bool {
                Rcomplex v = COMPLEX_ELT(vector, index);
                return (ISNAN(v.r) || ISNAN(v.i));
            });
    }

    void visit(const tastr::ast::DoubleAScalarTypeNode& node) override final {
        SEXP value = pop_value_();
        satisfies_vector_or_scalar_(
            value, REALSXP, [](SEXP vector, int index) -> bool {
                return ISNAN(REAL_ELT(vector, index));
            });
    }

    void visit(const tastr::ast::IntegerAScalarTypeNode& node) override final {
        SEXP value = pop_value_();
        satisfies_vector_or_scalar_(
            value, INTSXP, [](SEXP vector, int index) -> bool {
                return INTEGER_ELT(vector, index) == NA_INTEGER;
            });
    }

    void visit(const tastr::ast::LogicalAScalarTypeNode& node) override final {
        SEXP value = pop_value_();
        satisfies_vector_or_scalar_(
            value, LGLSXP, [](SEXP vector, int index) -> bool {
                return LOGICAL_ELT(vector, index) == NA_LOGICAL;
            });
    }

    void visit(const tastr::ast::RawAScalarTypeNode& node) override final {
        SEXP value = pop_value_();
        satisfies_vector_or_scalar_(
            value, RAWSXP, [](SEXP vector, int index) -> bool {
                /* NOTE: no such thing as a raw
                 * NA */
                return false;
            });
    }

    void visit(const tastr::ast::EnvironmentTypeNode& node) override final {
        SEXP value = pop_value_();
        SEXPTYPE rtype = TYPEOF(value);
        push_result_(rtype == ENVSXP);
    }

    void visit(const tastr::ast::ExpressionTypeNode& node) override final {
        SEXP value = pop_value_();
        SEXPTYPE rtype = TYPEOF(value);
        push_result_(rtype == EXPRSXP);
    }

    void visit(const tastr::ast::LanguageTypeNode& node) override final {
        SEXP value = pop_value_();
        SEXPTYPE rtype = TYPEOF(value);
        push_result_(rtype == LANGSXP);
    }

    void visit(const tastr::ast::SymbolTypeNode& node) override final {
        SEXP value = pop_value_();
        SEXPTYPE rtype = TYPEOF(value);
        push_result_(rtype == SYMSXP);
    }

    void visit(const tastr::ast::ExternalPointerTypeNode& node) override final {
        SEXP value = pop_value_();
        SEXPTYPE rtype = TYPEOF(value);
        push_result_(rtype == EXTPTRSXP);
    }

    void visit(const tastr::ast::BytecodeTypeNode& node) override final {
        SEXP value = pop_value_();
        SEXPTYPE rtype = TYPEOF(value);
        push_result_(rtype == BCODESXP);
    }

    void visit(const tastr::ast::PairlistTypeNode& node) override final {
        SEXP value = pop_value_();
        SEXPTYPE rtype = TYPEOF(value);
        push_result_(rtype == LISTSXP);
    }

    void visit(const tastr::ast::S4TypeNode& node) override final {
        SEXP value = pop_value_();
        SEXPTYPE rtype = TYPEOF(value);
        push_result_(rtype == S4SXP);
    }

    void visit(const tastr::ast::WeakReferenceTypeNode& node) override final {
        SEXP value = pop_value_();
        SEXPTYPE rtype = TYPEOF(value);
        push_result_(rtype == WEAKREFSXP);
    }

    void visit(const tastr::ast::AnyTypeNode& node) override final {
        SEXP value = pop_value_();
        push_result_(true);
    }

    void visit(const tastr::ast::UnknownTypeNode& node) override final {
        SEXP value = pop_value_();
        push_result_(value == R_MissingArg);
    }

    void visit(const tastr::ast::ParameterNode& node) override final {
        SEXP value = pop_value_();
        tastr::ast::Node::count_t size = node.get_parameter_count();
        for (int i = 0; i < size; ++i) {
            push_value_(value);
            push_seq_index_(i);
            const tastr::ast::Node& child_node = node.at(i);
            child_node.accept(*this);
        }
        and_result_(size);
    }

    void visit(const tastr::ast::ListTypeNode& node) override final {
        SEXP value = pop_value_();

        bool result = TYPEOF(value) == VECSXP;

        if (!result) {
            push_result_(result);
        } else {
            const tastr::ast::ParameterNode& params = node.get_parameters();
            assert(params.size() == 1);
            const tastr::ast::Node& param_node = params.at(0);
            int list_size = LENGTH(value);
            for (int i = 0; i < list_size; ++i) {
                SEXP element = VECTOR_ELT(value, i);
                push_value_(element);
                param_node.accept(*this);
            }
            and_result_(list_size);
        }
    }

    void visit(const tastr::ast::StructTypeNode& node) override final {
        SEXP value = pop_value_();

        bool result = TYPEOF(value) == VECSXP;

        if (!result) {
            push_result_(result);
        } else {
            push_value_(value);
            node.get_parameters().accept(*this);
        }
    }

    void visit(const tastr::ast::TupleTypeNode& node) override final {
        SEXP value = pop_value_();
        bool result = TYPEOF(value) == VECSXP;

        if (!result) {
            push_result_(result);
        } else {
            push_value_(value);
            node.get_parameters().accept(*this);
        }
    }

    void visit(const tastr::ast::GroupTypeNode& node) override final {
        node.get_inner_type().accept(*this);
    }

    void visit(const tastr::ast::UnionTypeNode& node) override final {
        SEXP value = peek_value_();
        node.get_first_type().accept(*this);
        if (peek_result_()) {
            return;
        }
        pop_result_();
        push_value_(value);
        node.get_second_type().accept(*this);
    }

    void visit(const tastr::ast::NullTypeNode& node) override final {
        SEXP value = pop_value_();
        push_result_(value == R_NilValue);
    }

    void visit(const tastr::ast::NullableTypeNode& node) override final {
        SEXP value = pop_value_();

        bool result = value == R_NilValue;
        if (result) {
            push_result_(result);
        } else {
            push_value_(value);
            node.get_inner_type().accept(*this);
        }
    }

    void visit(const tastr::ast::TagTypePairNode& node) override final {
        SEXP list = pop_value_();
        seq_index_t seq_index = pop_seq_index_();
        SEXP names = getAttrib(list, R_NamesSymbol);
        SEXP name = STRING_ELT(names, seq_index);
        SEXP element = VECTOR_ELT(list, seq_index);
        push_value_(name);
        node.get_identifier().accept(*this);
        push_value_(element);
        node.get_type().accept(*this);
        and_result_();
    }

    void visit(const tastr::ast::FunctionTypeNode& node) override final {
        SEXP value = pop_value_();
        SEXPTYPE valtype = TYPEOF(value);
        bool result =
            valtype == CLOSXP || valtype == BUILTINSXP || valtype == SPECIALSXP;
        push_result_(result);
    }

    void visit(const tastr::ast::CommaSeparatorNode& node) override final {
        log_error("%s: this case should not occur", "CommaSeparatorNode");
    }

    void visit(const tastr::ast::VarargTypeNode& node) override final {
        log_error("%s: this case should not occur", "VarargTypeNode");
    }

    void visit(const tastr::ast::TypeDeclarationNode& node) override final {
        log_error("%s: this case should not occur", "TypeDeclarationNode");
    }

    void visit(const tastr::ast::TopLevelNode& node) override final {
        log_error("%s: this case should not occur", "TopLevelNode");
    }

    void visit(const tastr::ast::KeywordNode& node) override final {
        log_error("%s: this case should not occur", "KeywordNode");
    }

    void visit(const tastr::ast::OperatorNode& node) override final {
        log_error("%s: this case should not occur", "OperatorNode");
    }

    void visit(const tastr::ast::TerminatorNode& node) override final {
        log_error("%s: this case should not occur", "TerminatorNode");
    }

    void visit(const tastr::ast::SeparatorNode& node) override final {
        log_error("%s: this case should not occur", "SeparatorNode");
    }

    void visit(const tastr::ast::EmptyNode& node) override final {
        log_error("%s: this case should not occur", "EmptyNode");
    }

    void visit(const tastr::ast::EofNode& node) override final {
        log_error("%s: this case should not occur", "EofNode");
    }

  private:
    bool is_vector_type_(SEXP value) {
        SEXPTYPE sexptype = TYPEOF(value);
        return (sexptype == INTSXP || sexptype == REALSXP ||
                sexptype == RAWSXP || sexptype == LGLSXP || sexptype == STRSXP);
    };

    template <typename T>
    void satisfies_vector_or_scalar_(SEXP value, SEXPTYPE type, T check_na) {
        if (TYPEOF(value) != type) {
            push_result_(false);
            return;
        }

        int length = LENGTH(value);

        if (!expected_na_()) {
            for (int i = 0; i < length; ++i) {
                if (check_na(value, i)) {
                    push_result_(false);
                    return;
                }
            }
        }

        if (length > 1 && expected_scalar_()) {
            push_result_(false);
            return;
        }

        push_result_(true);
    }

    void push_result_(bool result) {
        result_stack_.push_back(result);
    }

    bool pop_result_() {
        bool result = result_stack_.back();
        result_stack_.pop_back();
        return result;
    }

    bool peek_result_() {
        bool result = result_stack_.back();
        return result;
    }

    SEXP peek_value_() {
        return value_stack_.back();
    }

    void push_value_(SEXP value) {
        value_stack_.push_back(value);
    }

    SEXP pop_value_() {
        SEXP value = value_stack_.back();
        value_stack_.pop_back();
        return value;
    }

    void and_result_(int n = 2) {
        bool result = true;

        for (int i = 0; i < n; ++i) {
            bool result1 = pop_result_();
            result = result && result1;
        }

        push_result_(result);
    }

    void or_result_(int n = 2) {
        bool result = false;

        for (int i = 0; i < n; ++i) {
            bool result1 = pop_result_();
            result = result && result1;
        }

        push_result_(result);
    }

    void set_na_() {
        na_ = true;
    }

    void unset_na_() {
        na_ = false;
    }

    bool expected_na_() {
        return na_;
    }

    void set_vector_() {
        vector_ = true;
    }

    void unset_vector_() {
        vector_ = false;
    }

    bool expected_scalar_() {
        return !vector_;
    }

    void push_seq_index_(seq_index_t seq_index) {
        seq_index_stack_.push_back(seq_index);
    }

    seq_index_t pop_seq_index_() {
        seq_index_t seq_index = seq_index_stack_.back();
        seq_index_stack_.pop_back();
        return seq_index;
    }

    std::vector<bool> result_stack_;
    std::vector<SEXP> value_stack_;
    std::vector<seq_index_t> seq_index_stack_;
    bool na_;
    bool vector_;
};

#endif /* CONTRACTR_TYPECHECKER_HPP */
