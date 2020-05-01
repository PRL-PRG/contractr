#ifndef CONTRACTR_TYPE_CHECKER_HPP
#define CONTRACTR_TYPE_CHECKER_HPP

#include "utilities.hpp"

#include <cassert>
#include <functional>
#include <algorithm>
#include <tastr/visitor/visitor.hpp>

class TypeChecker final: public tastr::visitor::ConstNodeVisitor {
  private:
    using seq_index_t = int;
    using name_element_t = std::pair<SEXP, SEXP>;

  public:
    TypeChecker(): ConstNodeVisitor(), result_(false) {
    }

    bool typecheck(const std::string& parameter_name,
                   SEXP value,
                   const tastr::ast::Node& node) {
        if (is_dot_dot_dot_parameter_(parameter_name)) {
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

        if (value == NA_STRING) {
            push_result_(node.is_missing());
        } else {
            std::string name = node.get_name();
            push_result_(CHAR(value) == name);
        }
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
                return (v.r == NA_REAL) || (v.i == NA_REAL);
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
        SEXPTYPE rtype = type_of_sexp(value);
        push_result_(rtype == ENVSXP);
    }

    void visit(const tastr::ast::ExpressionTypeNode& node) override final {
        SEXP value = pop_value_();
        SEXPTYPE rtype = type_of_sexp(value);
        push_result_(rtype == EXPRSXP);
    }

    void visit(const tastr::ast::LanguageTypeNode& node) override final {
        SEXP value = pop_value_();
        SEXPTYPE rtype = type_of_sexp(value);
        push_result_(rtype == LANGSXP);
    }

    void visit(const tastr::ast::SymbolTypeNode& node) override final {
        SEXP value = pop_value_();
        SEXPTYPE rtype = type_of_sexp(value);
        push_result_(rtype == SYMSXP);
    }

    void visit(const tastr::ast::ExternalPointerTypeNode& node) override final {
        SEXP value = pop_value_();
        SEXPTYPE rtype = type_of_sexp(value);
        push_result_(rtype == EXTPTRSXP);
    }

    void visit(const tastr::ast::DataFrameTypeNode& node) override final {
        SEXP value = pop_value_();
        push_result_(is_data_frame(value));
    }

    void visit(const tastr::ast::BytecodeTypeNode& node) override final {
        SEXP value = pop_value_();
        SEXPTYPE rtype = type_of_sexp(value);
        push_result_(rtype == BCODESXP);
    }

    void visit(const tastr::ast::PairlistTypeNode& node) override final {
        SEXP value = pop_value_();
        SEXPTYPE rtype = type_of_sexp(value);
        push_result_(rtype == LISTSXP);
    }

    void visit(const tastr::ast::S4TypeNode& node) override final {
        SEXP value = pop_value_();
        SEXPTYPE rtype = type_of_sexp(value);
        push_result_(rtype == S4SXP);
    }

    void visit(const tastr::ast::WeakReferenceTypeNode& node) override final {
        SEXP value = pop_value_();
        SEXPTYPE rtype = type_of_sexp(value);
        push_result_(rtype == WEAKREFSXP);
    }

    void visit(const tastr::ast::AnyTypeNode& node) override final {
        SEXP value = pop_value_();
        push_result_(true);
    }

    void visit(const tastr::ast::UnknownTypeNode& node) override final {
        SEXP value = pop_value_();
        SEXPTYPE rtype = type_of_sexp(value);
        push_result_(rtype == MISSINGSXP);
    }

    void visit(const tastr::ast::ParameterNode& node) override final {
        SEXP value = pop_value_();
        tastr::ast::Node::count_t size = node.get_parameter_count();

        if (size != LENGTH(value)) {
            push_result_(false);
        }

        else {
            for (int i = 0; i < size; ++i) {
                push_value_(value);
                push_seq_index_(i);
                const tastr::ast::Node& child_node = node.at(i);
                child_node.accept(*this);
            }
            and_result_(size);
        }
    }

    void visit(const tastr::ast::ListTypeNode& node) override final {
        SEXP value = pop_value_();

        bool result = type_of_sexp(value) == VECSXP;

        if (!result) {
            push_result_(result);
        }

        else {
            int list_size = LENGTH(value);
            const tastr::ast::ParameterNode& params = node.get_parameters();

            if (params.get_parameter_count() == list_size == 0) {
                push_result_(true);
            }

            else {
                const tastr::ast::Node& param_node = params.at(0);
                for (int i = 0; i < list_size; ++i) {
                    SEXP element = VECTOR_ELT(value, i);
                    push_value_(element);
                    param_node.accept(*this);
                }
                and_result_(list_size);
            }
        }
    }

    void visit(const tastr::ast::StructTypeNode& node) override final {
        SEXP value = pop_value_();

        bool result = type_of_sexp(value) == VECSXP;

        if (!result) {
            push_result_(result);
        } else {
            typecheck_struct_(value, node.get_parameters());
        }
    }

    void visit(const tastr::ast::TupleTypeNode& node) override final {
        SEXP value = pop_value_();
        bool result = type_of_sexp(value) == VECSXP;

        if (!result) {
            push_result_(result);
        }

        else {
            const tastr::ast::ParameterNode& param_node(node.get_parameters());
            tastr::ast::Node::count_t size = param_node.get_parameter_count();

            int list_size = LENGTH(value);

            if (list_size != size) {
                push_result_(false);
            }

            else {
                for (int i = 0; i < size; ++i) {
                    SEXP element = VECTOR_ELT(value, i);
                    push_value_(element);
                    const tastr::ast::Node& child_node = param_node.at(i);
                    child_node.accept(*this);
                }
                and_result_(size);
            }
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
        SEXP names = Rf_getAttrib(list, R_NamesSymbol);

        /* list names can be non-string values */
        if (TYPEOF(names) != STRSXP) {
            push_result_(false);
        } else {
            SEXP name = STRING_ELT(names, seq_index);
            SEXP element = VECTOR_ELT(list, seq_index);
            push_value_(name);
            node.get_identifier().accept(*this);
            push_value_(element);
            node.get_type().accept(*this);
            and_result_();
        }
    }

    void visit(const tastr::ast::FunctionTypeNode& node) override final {
        SEXP value = pop_value_();
        SEXPTYPE valtype = type_of_sexp(value);
        bool result =
            valtype == CLOSXP || valtype == BUILTINSXP || valtype == SPECIALSXP;
        push_result_(result);
    }

    void visit(const tastr::ast::CommaSeparatorNode& node) override final {
        Rf_errorcall(R_NilValue,
                     "typechecker encountered unexpected '%s'",
                     "CommaSeparatorNode");
    }

    void visit(const tastr::ast::VarargTypeNode& node) override final {
        Rf_errorcall(R_NilValue,
                     "typechecker encountered unexpected '%s'",
                     "VarargTypeNode");
    }

    void visit(const tastr::ast::TypeDeclarationNode& node) override final {
        Rf_errorcall(R_NilValue,
                     "typechecker encountered unexpected '%s'",
                     "TypeDeclarationNode");
    }

    void visit(const tastr::ast::TopLevelNode& node) override final {
        Rf_errorcall(R_NilValue,
                     "typechecker encountered unexpected '%s'",
                     "TopLevelNode");
    }

    void visit(const tastr::ast::KeywordNode& node) override final {
        Rf_errorcall(R_NilValue,
                     "typechecker encountered unexpected '%s'",
                     "KeywordNode");
    }

    void visit(const tastr::ast::OperatorNode& node) override final {
        Rf_errorcall(R_NilValue,
                     "typechecker encountered unexpected '%s'",
                     "OperatorNode");
    }

    void visit(const tastr::ast::TerminatorNode& node) override final {
        Rf_errorcall(R_NilValue,
                     "typechecker encountered unexpected '%s'",
                     "TerminatorNode");
    }

    void visit(const tastr::ast::SeparatorNode& node) override final {
        Rf_errorcall(R_NilValue,
                     "typechecker encountered unexpected '%s'",
                     "SeparatorNode");
    }

    void visit(const tastr::ast::EmptyNode& node) override final {
        Rf_errorcall(
            R_NilValue, "typechecker encountered unexpected '%s'", "EmptyNode");
    }

    void visit(const tastr::ast::EofNode& node) override final {
        Rf_errorcall(
            R_NilValue, "typechecker encountered unexpected '%s'", "EofNode");
    }

  private:
    void typecheck_struct_(SEXP list,
                           const tastr::ast::ParameterNode& parameter_node) {
        SEXP list_names = getAttrib(list, R_NamesSymbol);
        int parameter_size = parameter_node.get_parameter_count();
        int list_size = LENGTH(list);

        /* if list does not have names, it cannot be a struct */
        if (list_names == R_NilValue) {
            push_result_(false);
        }
        /*  if list names are not as long as the list, it cannot be a struct */
        else if (LENGTH(list_names) < list_size) {
            push_result_(false);
        }
        /* base case; all lists are subtypes of structs of size 0 */
        else if (parameter_size == 0) {
            push_result_(true);
        }
        /* if list size is smaller than parameter size, then definitely it won't
           have all the elements and it cannot be a subtype  */
        else if (list_size < parameter_size) {
            push_result_(false);
        }
        /* walk over all struct parameters in the type and match against list
           elements  */
        else {
            std::vector<name_element_t> rlist;

            /* convert r list to custom representation for sorting */
            for (int i = 0; i < list_size; ++i) {
                SEXP name = STRING_ELT(list_names, i);
                SEXP element = VECTOR_ELT(list, i);
                rlist.push_back({name, element});
            }

            auto comparator = [](name_element_t& a, name_element_t& b) -> bool {
                if (a.first == NA_STRING) {
                    return true;
                } else if (b.first == NA_STRING) {
                    return false;
                } else {
                    return std::string(CHAR(a.first)) <
                           std::string(CHAR(b.first));
                }
            };

            /* sort rlist by name for quick search  */
            std::sort(rlist.begin(), rlist.end(), comparator);

            /*  go through all the struct elements and search for list elements
             * that match that name */

            bool result = true;

            for (int i = 0; i < parameter_size; ++i) {
                const tastr::ast::TagTypePairNode& pair_node =
                    tastr::ast::as<tastr::ast::TagTypePairNode>(
                        parameter_node.at(i));

                const tastr::ast::IdentifierNode& identifier(
                    pair_node.get_identifier());
                const tastr::ast::Node& type_node = pair_node.get_type();

                int index = binary_search_(rlist, identifier);

                /* if element with this name is not found; bail out */
                if (index == -1) {
                    result = false;
                    break;
                }

                /* if element with the name is found, check for all consecutive
                 * elements with this name since list can have repeated elements
                 */
                do {
                    SEXP element = rlist[index].second;

                    push_value_(element);
                    type_node.accept(*this);

                    /* if an element matched by name but the type did not match;
                     * bail out early */
                    if (!pop_result_()) {
                        result = false;
                        break;
                    }

                    ++index;
                } while (index < list_size &&
                         match_names_(identifier, rlist[index].first) == 0);

                /*  if the do loop above set result to false then bail out */
                if (!result) {
                    break;
                }
            }

            push_result_(result);
        }
    }

    int binary_search_(const std::vector<name_element_t>& rlist,
                       const tastr::ast::IdentifierNode& identifier) {
        int left = 0;
        int right = rlist.size() - 1;
        int mid = 0;

        while (left <= right) {
            mid = left + (right - left) / 2;

            int match = match_names_(identifier, rlist[mid].first);

            if (match == 0) {
                return mid;
            } else if (match < 1) {
                right = mid - 1;
            } else {
                left = mid + 1;
            }
        }
        return -1;
    }

    int match_names_(const tastr::ast::IdentifierNode& identifier, SEXP name) {
        if (identifier.is_missing()) {
            if (name == NA_STRING) {
                return 0;
            } else {
                return -1;
            }
        } else if (name == NA_STRING) {
            return 1;
        } else if (identifier.get_name() < CHAR(name)) {
            return -1;
        } else if (identifier.get_name() > CHAR(name)) {
            return 1;
        } else {
            return 0;
        }
    }

    bool is_dot_dot_dot_parameter_(const std::string& parameter_name) {
        return parameter_name == "...";
    }

    bool is_vector_type_(SEXP value) {
        SEXPTYPE sexptype = type_of_sexp(value);
        return (sexptype == INTSXP || sexptype == REALSXP ||
                sexptype == CPLXSXP || sexptype == RAWSXP ||
                sexptype == LGLSXP || sexptype == STRSXP);
    };

    template <typename T>
    void satisfies_vector_or_scalar_(SEXP value, SEXPTYPE type, T check_na) {
        if (type_of_sexp(value) != type) {
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

        if (length != 1 && expected_scalar_()) {
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

    bool result_;
    std::vector<bool> result_stack_;
    std::vector<SEXP> value_stack_;
    std::vector<seq_index_t> seq_index_stack_;
    bool na_;
    bool vector_;
};

#endif /* CONTRACTR_TYPE_CHECKER_HPP */
