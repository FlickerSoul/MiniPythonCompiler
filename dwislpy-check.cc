#include <variant>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include "dwislpy-check.hh"
#include "dwislpy-ast.hh"
#include "dwislpy-util.hh"

const Type BOOL_T = BoolTy {}; 
const Type INT_T = IntTy {};
const Type STR_T = StrTy {};
const Type NONE_T = NoneTy {};

std::string get_type_str(Type type){
    if (std::holds_alternative<IntTy>(type)) {
        return "int";
    } else if (std::holds_alternative<StrTy>(type)) {
        return "str";
    } else if (std::holds_alternative<BoolTy>(type)) {
        return "bool";
    } else if (std::holds_alternative<NoneTy>(type)) {
        return "None";
    } else {
        return "unknown";
    }
}

bool is_int(Type type) {
    return std::holds_alternative<IntTy>(type);
}

bool is_str(Type type) {
    return std::holds_alternative<StrTy>(type);
}

bool is_bool(Type type) {
    return std::holds_alternative<BoolTy>(type);
}

bool is_None(Type type) {
    return std::holds_alternative<NoneTy>(type);
}

bool operator==(Type type1, Type type2) {
    if (is_int(type1) && is_int(type2)) {
        return true;
    }
    if (is_str(type1) && is_str(type2)) {
        return true;
    }
    if (is_bool(type1) && is_bool(type2)) {
        return true;
    }
    if (is_None(type1) && is_None(type2)) {
        return true;
    }
    return false;
}

bool operator!=(Type type1, Type type2) {
    return !(type1 == type2);
}

std::string type_name(Type type) {
    if (is_int(type)) {
        return "int";
    }
    if (is_str(type)) {
        return "str";
    }
    if (is_bool(type)) {
        return "bool";
    } 
    if (is_None(type)) {
        return "None";
    }
    return "wtf";
}

unsigned int Defn::arity(void) const {
    return args.get_frmls_size();
}

Type Defn::returns(void) const {
    return ret_type;
}

SymInfo_ptr Defn::formal(int i) const {
    return args.get_frml(i);
}

void Prgm::chck(void) {
    for (std::pair<Name,Defn_ptr> dfpr : defs) {
        dfpr.second->chck(defs);
    }
    if (main) {
        Rtns rtns = main->chck(Rtns{Void {}},defs, main_symt);
        if (!std::holds_alternative<Void>(rtns)) {
            DwislpyError(main->where(), "Main script should not return.");
        }
    }

}

Type type_of(Rtns rtns) {
    if (std::holds_alternative<VoidOr>(rtns)) {
        return std::get<VoidOr>(rtns).type;
    }
    if (std::holds_alternative<Type>(rtns)) {
        return std::get<Type>(rtns);
    }
    return NONE_T; // Should not happen.
}


void Defn::chck(Defs& defs) {
    Rtns rtns = body->chck(Rtns{ret_type}, defs, args);

    if (std::holds_alternative<Void>(rtns)) {
        throw DwislpyError(body->where(), "Definition body never returns.");
    }
    if (std::holds_alternative<VoidOr>(rtns)) {
        throw DwislpyError(body->where(), "Definition body might not return.");
    }

    if (type_of(rtns) != ret_type) {
        throw DwislpyError(body->where(), "Definition body returns " + get_type_str(type_of(rtns)) + " but should return " + get_type_str(ret_type));
    }
}

Rtns Blck::chck(Rtns expd, Defs& defs, SymT& symt) {

    symt.mark();

    // Scan through the statements and check their return behavior.
    Rtns rtns = Void {};
    for (Stmt_ptr stmt : stmts) {
        
        // Check this statement.
        [[maybe_unused]]
        Rtns stmt_rtns = stmt->chck(expd, defs, symt);

        if (std::holds_alternative<Type>(stmt_rtns)) {
            if (!std::holds_alternative<Void>(rtns) && type_of(rtns) != type_of(stmt_rtns)) {
                throw DwislpyError(stmt->where(), "Statement returns " + get_type_str(type_of(stmt_rtns)) + " but previous statement returns " + get_type_str(type_of(rtns)) + ".");
            }

            rtns = stmt_rtns;
            break;
        } else if (std::holds_alternative<VoidOr>(stmt_rtns)) {
            if (std::holds_alternative<Void>(rtns)) {
                rtns = stmt_rtns;
            } else {
                if (type_of(rtns) != type_of(stmt_rtns)) {
                    throw DwislpyError(stmt->where(), "Statement might return different types.");
                }
            }
        }
    }            

    symt.pop_until_mark();

    return rtns;
}

Rtns Asgn::chck([[maybe_unused]] Rtns expd, Defs& defs, SymT& symt) {
    if (!symt.has_info(name)) {
        throw DwislpyError(where(), "Variable '" + name + "' never introduced.");
    }
    Type name_ty = symt.get_info(name)->type;
    Type expn_ty = expn->chck(defs,symt);
    if (name_ty != expn_ty) {
        std::string msg = "Type mismatch. Expected expression of type ";
        msg += type_name(name_ty) + ".";
        throw DwislpyError {expn->where(), msg};
    }
    return Rtns {Void {}};
}

Rtns Pass::chck([[maybe_unused]] Rtns expd,
                [[maybe_unused]] Defs& defs,
                [[maybe_unused]] SymT& symt) {
    return Rtns {Void {}};
}

Rtns Prnt::chck([[maybe_unused]] Rtns expd, Defs& defs, SymT& symt) {
    for (auto expn : expns) {
        [[maybe_unused]] Type expn_ty = expn->chck(defs,symt);
    }
    return Rtns {Void {}};
}

Rtns Ntro::chck([[maybe_unused]] Rtns expd, Defs& defs, SymT& symt) {
    if (symt.is_redefining(name)) {
        throw DwislpyError(where(), "Variable '" + name + "' already introduced.");
    }

    auto expn_type = expn->chck(defs, symt);
    symt.add_locl(name, expn_type);
    return Rtns {Void {}}; 
}

Rtns FRtn::chck(Rtns expd,
                [[maybe_unused]] Defs& defs, [[maybe_unused]] SymT& symt) {

    Type expd_ty = type_of(expd);

    if ((expn.get() == nullptr)) {
        throw DwislpyError(where(), "The return value does not exist but should return " + get_type_str(expd_ty));
    } else {
        auto actual_tp = expn->chck(defs, symt);
        
        if (expd_ty == actual_tp) {
            return Rtns { expd_ty };
        }

        throw DwislpyError(where(), "The return type should be " + get_type_str(expd_ty) + " but got " + get_type_str(actual_tp));
    }
}

Rtns PRtn::chck(Rtns expd,
                [[maybe_unused]] Defs& defs, [[maybe_unused]] SymT& symt) {
    if (std::holds_alternative<Void>(expd)) {
        throw DwislpyError {where(), "Unexpected return statement."};
    }
    Type expd_ty = type_of(expd);
    if (!is_None(expd_ty)) {
        throw DwislpyError {where(), "A procedure does not return a value."};
    }
    return  NONE_T;
}


Rtns PCll::chck([[maybe_unused]] Rtns expd, Defs& defs, SymT& symt) {
    //
    // This should look up a procedure's definition. It should check:
    // * that the correct number of arguments are passed.
    // * that the return type matches 
    // * that each of the argument expressions type check
    // * that the type of each argument matches the type signature
    //
    // Fix this!!!

    auto fn_iter = defs.find(name);
    
    if (fn_iter == defs.end()) throw DwislpyError(where(), "fn " + name + " does not exsits in the context");
    
    Defn_ptr fn = fn_iter->second;

    if (fn->arity() != args.size()) 
        throw DwislpyError {where(), "fn  needs " + std::to_string(fn->arity()) + " number of args but got " + std::to_string(args.size())};

    for (std::size_t i = 0; i < fn->arity(); i++) {
        auto fm_arg_tp = fn->args.get_frml(i)->type;
        auto ac_arg_tp = args[i]->chck(defs, symt);
        if (fm_arg_tp != ac_arg_tp) {
            throw DwislpyError(where(), "The argument at " + std::to_string(i) + " should have type " + get_type_str(fm_arg_tp) + " but got " + get_type_str(ac_arg_tp));
        }
    }

    return Void {};
}

bool has_void(Rtns rtns) {
    return std::holds_alternative<Void>(rtns) || std::holds_alternative<VoidOr>(rtns);
}

Rtns Ifcd::chck(Rtns expd, Defs &defs, SymT &symt) { throw DwislpyError(where(), "should not be called directly");}

Rtns Else::chck(Rtns expd, Defs &defs, SymT &symt) { throw DwislpyError(where(), "should not be called directly");}

Rtns Elif::chck(Rtns expd, Defs &defs, SymT &symt) { throw DwislpyError(where(), "should not be called directly");}

Rtns Cond::chck(Rtns expd, Defs& defs, SymT& symt) {
    //
    // This should check that the condition is a bool.
    // It should check each of the two blocks return behavior.
    // It should summarize the return behavior.
    //

    if (ifcds.size() == 0) throw DwislpyError(where(), "no if condition");

    for (std::size_t i = 0; i < ifcds.size(); i++) {
        auto cond_tp = ifcds[i]->cond->chck(defs, symt);
        if (cond_tp != BOOL_T) throw DwislpyError(where(), "The condition at " + std::to_string(i) + " should have type Bool but got " + get_type_str(cond_tp));
    }

    Rtns return_pt = ifcds[0]->body->chck(expd, defs, symt);
    auto return_tp = type_of(return_pt);
    const bool rt_is_void = std::holds_alternative<Void>(return_pt);
    const bool rt_has_void = has_void(return_pt);

    for (std::size_t i = 1; i < ifcds.size(); i++) {
        auto body_rt_tp = ifcds[0]->body->chck(expd, defs, symt);

        const bool body_has_void = has_void(body_rt_tp);
        const bool body_is_void = std::holds_alternative<Void>(body_rt_tp);
        
        if (body_is_void) {
            if (!rt_is_void) {
                return_pt = VoidOr { type_of(return_pt) };
            }
        } else {
            auto body_tp = type_of(body_rt_tp);
            if (return_tp != body_tp) {
                throw DwislpyError(where(), "The return type at " + std::to_string(i) + " if should have type " + get_type_str(return_tp) + " but got " + get_type_str(body_tp));
            }

            if (rt_has_void || body_has_void) {
                return_pt = VoidOr { return_tp };
            }
        }
    }

    auto body_rt_tp = els->body->chck(expd, defs, symt);

    const bool body_has_void = has_void(body_rt_tp);
    const bool body_is_void = std::holds_alternative<Void>(body_rt_tp);

    if (body_is_void) {
        if (!rt_is_void) {
            return_pt = VoidOr { type_of(return_pt) };
        }
    } else {
        auto body_tp = type_of(body_rt_tp);
        if (return_tp != body_tp) {
            throw DwislpyError(where(), "The return type at else should have type " + get_type_str(return_tp) + " but got " + get_type_str(body_tp));
        }

        if (rt_has_void || body_has_void) {
            return_pt = VoidOr { return_tp };
        }
    }
    return return_pt;
}

Rtns Whil::chck(Rtns expd, Defs& defs, SymT& symt) {
    //
    // This should check that the condition is a bool.
    // It should check the block's return behavior.
    // It should summarize the return behavior. It shouldv be Void
    // or VoidOr because loop bodies don't always execute.

    auto cond_tp = cond->chck(defs, symt);
    if (cond_tp != BOOL_T) 
        throw DwislpyError(where(), "The condition for while should have type Bool but got " + get_type_str(cond_tp));

    auto body_tp = body->chck(expd, defs, symt);
    if (std::holds_alternative<Void>(body_tp)) {
        return Void {};
    } else {
        return VoidOr { type_of (body_tp) };
    }
}

Rtns Rept::chck(Rtns expd, Defs &defs, SymT &symt) {
    auto cond_tp = cond->chck(defs, symt);
    if (cond_tp != BOOL_T) throw DwislpyError(where(), "The condition for repeat should have type Bool but got " + get_type_str(cond_tp));

    auto body_tp = body->chck(expd, defs, symt);
    if (std::holds_alternative<Void>(body_tp)) {
        return Void {};
    } else {
        return VoidOr { type_of (body_tp) };
    }
}

Type Plus::chck(Defs& defs, SymT& symt) {
    Type left_ty = left->chck(defs,symt);
    Type rght_ty = rght->chck(defs,symt);
    if (is_int(left_ty) && is_int(rght_ty)) {
        return INT_T;
    } else if (is_str(left_ty) && is_str(rght_ty)) {
        return STR_T;
    } else {
        std::string msg = "Wrong operand types for plus.";
        throw DwislpyError { where(), msg };
    }
}

Type Mnus::chck(Defs& defs, SymT& symt) {
    // Doesn't check subexpressions.
    Type left_ty = left->chck(defs,symt);
    Type rght_ty = rght->chck(defs,symt);
    if (is_int(left_ty) && is_int(rght_ty)) {
        return INT_T;
    } else {
        std::string msg = "Wrong operand types for minus.";
        throw DwislpyError { where(), msg };
    }
}

Type Tmes::chck(Defs& defs, SymT& symt) {
    // Doesn't check subexpressions.
    Type left_ty = left->chck(defs,symt);
    Type rght_ty = rght->chck(defs,symt);
    if (is_int(left_ty) && is_int(rght_ty)) {
        return INT_T;
    } else if (is_str(left_ty) && is_int(rght_ty)) {
        return STR_T;
    } else {
        std::string msg = "Wrong operand types for times.";
        throw DwislpyError { where(), msg };
    }
}

Type IDiv::chck(Defs& defs, SymT& symt) {
    // Doesn't check subexpressions.
    // Fix this!!
    Type left_ty = left->chck(defs,symt);
    Type rght_ty = rght->chck(defs,symt);
    if (is_int(left_ty) && is_int(rght_ty)) {
        return INT_T;
    } else {
        std::string msg = "Wrong operand types for div.";
        throw DwislpyError { where(), msg };
    }
}

Type IMod::chck(Defs& defs, SymT& symt) {
    // Doesn't check subexpressions.
    Type left_ty = left->chck(defs,symt);
    Type rght_ty = rght->chck(defs,symt);
    if (is_int(left_ty) && is_int(rght_ty)) {
        return INT_T;
    } else {
        std::string msg = "Wrong operand types for mod.";
        throw DwislpyError { where(), msg };
    }
}

Type Imus::chck(Defs &defs, SymT &symt) {
    Type expn_ty = expn->chck(defs,symt);
    if (is_int(expn_ty)) {
        return INT_T;
    } else {
        std::string msg = "Wrong operand types for inplace minus.";
        throw DwislpyError { where(), msg };
    }
}

Rtns Pleq::chck([[maybe_unused]]Rtns expd, Defs& defs, SymT& symt) {
    if (!symt.has_info(name)) {
        std::string msg = "The variable " + name + " is not defined.";
        throw DwislpyError { where(), msg };
    }
    auto stored = symt.get_info(name);
    Type left_ty = stored->type;
    Type rght_ty = expn->chck(defs,symt);
    if (is_int(left_ty) && is_int(rght_ty)) {
        return Void {};
    } else if (is_str(left_ty) && is_str(rght_ty)) {
        return Void {};
    }
    else {
        std::string msg = "Wrong operand types for plus eq.";
        throw DwislpyError { where(), msg };
    }
}

Rtns Mneq::chck([[maybe_unused]]Rtns expd, Defs& defs, SymT& symt) {
    if (!symt.has_info(name)) {
        std::string msg = "The variable " + name + " is not defined.";
        throw DwislpyError { where(), msg };
    }
    auto stored = symt.get_info(name);
    Type left_ty = stored->type;
    Type rght_ty = expn->chck(defs,symt);
    if (is_int(left_ty) && is_int(rght_ty)) {
        return Void {};
    } else {
        std::string msg = "Wrong operand types for minus eq.";
        throw DwislpyError { where(), msg };
    }
}

Type Cmlt::chck(Defs& defs, SymT& symt) {
    Type left_ty = lft->chck(defs,symt);
    Type rght_ty = rht->chck(defs,symt);
    if (is_int(left_ty) && is_int(rght_ty)) {
    } else {
        std::string msg = "Wrong operand types for <.";
        throw DwislpyError { where(), msg };
    }
    return BOOL_T; 
}

Type Inif::chck(Defs &defs, SymT &symt) {
    Type cond_ty = cond->chck(defs,symt);
    if (cond_ty != BOOL_T) {
        throw DwislpyError{where(), "The condition should be bool"};
    }

    Type if_br_ty = if_br->chck(defs,symt);
    Type else_br_ty = else_br->chck(defs,symt);

    if (if_br_ty != else_br_ty) {
        throw DwislpyError{where(), "The branches should have the same type"};
    }

    return if_br_ty;
}

Type Cmle::chck(Defs& defs, SymT& symt) {
    Type left_ty = lft->chck(defs,symt);
    Type rght_ty = rht->chck(defs,symt);
    if (is_int(left_ty) && is_int(rght_ty)) {
    } else {
        std::string msg = "Wrong operand types for <=.";
        throw DwislpyError { where(), msg };
    }
    return BOOL_T; 
}

Type Cmeq::chck(Defs& defs, SymT& symt) {
    Type left_ty = lft->chck(defs,symt);
    Type rght_ty = rht->chck(defs,symt);
    if (is_int(left_ty) && is_int(rght_ty)) {
    } else {
        std::string msg = "Wrong operand types for ==.";
        throw DwislpyError { where(), msg };
    }
    return BOOL_T; 
}

Type Cmge::chck(Defs& defs, SymT& symt) {
    Type left_ty = lft->chck(defs,symt);
    Type rght_ty = rht->chck(defs,symt);
    if (is_int(left_ty) && is_int(rght_ty)) {
    } else {
        std::string msg = "Wrong operand types for >=.";
        throw DwislpyError { where(), msg };
    }
    return BOOL_T; 
}

Type Cmgt::chck(Defs& defs, SymT& symt) {
    Type left_ty = lft->chck(defs,symt);
    Type rght_ty = rht->chck(defs,symt);
    if (is_int(left_ty) && is_int(rght_ty)) {
    } else {
        std::string msg = "Wrong operand types for >.";
        throw DwislpyError { where(), msg };
    }
    return BOOL_T; 
}
Type Conj::chck(Defs& defs, SymT& symt) {
    Type left_ty = lft->chck(defs,symt);
    Type rght_ty = rht->chck(defs,symt);
    if (is_bool(left_ty) && is_bool(rght_ty)) {
    } else {
        std::string msg = "Wrong operand types for and.";
        throw DwislpyError { where(), msg };
    }
    return BOOL_T; 
}

Type Disj::chck(Defs& defs, SymT& symt) {
    Type left_ty = lft->chck(defs,symt);
    Type rght_ty = rht->chck(defs,symt);
    if (is_bool(left_ty) && is_bool(rght_ty)) {
    } else {
        std::string msg = "Wrong operand types for or.";
        throw DwislpyError { where(), msg };
    }
    return BOOL_T; 
}

Type Negt::chck(Defs& defs, SymT& symt) {
    Type expr_ty = expn->chck(defs,symt);
    if (is_bool(expr_ty)) {
    } else {
        std::string msg = "Wrong operand types for not.";
        throw DwislpyError { where(), msg };
    }
    return BOOL_T; 
}

Type Ltrl::chck([[maybe_unused]] Defs& defs, [[maybe_unused]] SymT& symt) {
    if (std::holds_alternative<int>(valu)) {
        return INT_T;
    } else if (std::holds_alternative<std::string>(valu)) {
        return STR_T;
    } if (std::holds_alternative<bool>(valu)) {
        return BOOL_T;
    } else {
        return NONE_T;
    } 
}

Type Lkup::chck([[maybe_unused]] Defs& defs, SymT& symt) {
    if (symt.has_info(name)) {
        return symt.get_info(name)->type;
    } else {
        throw DwislpyError {where(), "Unknown identifier " + name + "."};
    } 
}

Type Inpt::chck(Defs& defs, SymT& symt) {
    Type expr_ty = expn->chck(defs,symt);
    if (!is_str(expr_ty)) {
        throw DwislpyError {where(), "Input requires a string."};
    }
    return STR_T; 
}

Type IntC::chck(Defs& defs, SymT& symt) {
    Type expr_ty = expn->chck(defs,symt);
    if (!is_str(expr_ty) || !is_int(expr_ty)) {
        throw DwislpyError {where(), "Input requires a string or integer."};
    }
    return INT_T; 
}

Type StrC::chck(Defs& defs, SymT& symt) {
    expn->chck(defs,symt);
    return STR_T; 
}


Type FCll::chck(Defs& defs, SymT& symt) {
    //
    // This should look up a function's definition. It should check:
    // * that the correct number of arguments are passed.
    // * that each of the argument expressions type check
    // * that the type of each argument matches the type signature
    // It should report the return type of the function.
    //
    auto fn_iter = defs.find(name);
    
    if (fn_iter == defs.end()) throw DwislpyError(where(), "fn " + name + " does not exsits in the context");
    
    Defn_ptr fn = fn_iter->second;

    if (fn->arity() != args.size()) throw DwislpyError {where(), "fn  needs " + std::to_string(fn->arity()) + " number of args but got " + std::to_string(args.size())};

    for (std::size_t i = 0; i < fn->arity(); i++) {
        auto fm_arg_tp = fn->args.get_frml(i)->type;
        auto ac_arg_tp = args[i]->chck(defs, symt);
        if (fm_arg_tp != ac_arg_tp) {
            throw DwislpyError(where(), "The argument at " + std::to_string(i) + " should have type " + get_type_str(fm_arg_tp) + "but got " + get_type_str(ac_arg_tp));
        }
    }

    return fn->ret_type;
}
