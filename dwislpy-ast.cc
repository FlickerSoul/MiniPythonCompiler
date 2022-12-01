#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>
#include <memory>
#include <utility>
#include <functional>
#include <iostream>
#include <exception>
#include <algorithm>
#include <sstream>

#include "dwislpy-ast.hh"
#include "dwislpy-check.hh"
#include "dwislpy-util.hh"

const std::string DEFAULT_INDENT_STR = "    ";

//
// dwislpy-ast.cc
//
// Below are the implementations of methods for AST nodes. They are organized
// into groups. The first group represents the DWISLPY interpreter by giving
// the code for 
//
//    Prgm::run, Blck::exec, Stmt::exec, Expn::eval
//
// The second group AST::output and AST::dump performs pretty printing
// of SLPY code and also the output of the AST, resp.
//

//
// to_string
//
// Helper function that converts a DwiSlpy value into a string.
// This is meant to be used by `print` and also `str`.
// 
std::string to_string(Valu v) {
    if (std::holds_alternative<int>(v)) {
        return std::to_string(std::get<int>(v));
    } else if (std::holds_alternative<std::string>(v)) {
        return std::get<std::string>(v);
    } else if (std::holds_alternative<bool>(v)) {
        if (std::get<bool>(v)) {
            return "True";
        } else {
            return "False";
        }
    } else if (std::holds_alternative<none>(v)) {
        return "None";
    } else {
        return "<unknown>";
    }
}

//
// to_repr
//
// Helper function that converts a DwiSlpy value into a string.
// This is meant to mimic Python's `repr` function in that it gives
// the source code string for the value. Used by routines that dump
// a literal value.
//
std::string to_repr(Valu v) {
    if (std::holds_alternative<std::string>(v)) {
        //
        // Strings have to be converted to show their quotes and also
        // to have the unprintable chatacters given as \escape sequences.
        //
        return "\"" + re_escape(std::get<std::string>(v)) + "\"";
    } else {
        //
        // The other types aren't special. (This will have to change when
        // we have value types that aren't "ground" types e.g. list types.)
        //
        return to_string(v);
    }
}



// * * * * *
// The DWISLPY interpreter
//

//
// Prgm::run, Blck::exec, Stmt::exec
//
//  - execute DWISLPY statements, changing the runtime context mapping
//    variables to their current values.
//

void Prgm::run(void) const {
    Ctxt main_ctxt { };
    if (main) {
        main->exec(defs,main_ctxt);
    }
}


std::optional<Valu> Blck::exec(const Defs& defs, Ctxt& ctxt) const {
    for (Stmt_ptr s : stmts) {
        std::optional<Valu> rv = s->exec(defs,ctxt);
        if (rv.has_value()) {
            return rv;
        } 
    }
    return std::nullopt;
}

std::optional<Valu> Asgn::exec(const Defs& defs,
                               Ctxt& ctxt) const {
    ctxt[name] = expn->eval(defs,ctxt);
    return std::nullopt;
}

std::optional<Valu> Ntro::exec(const Defs &defs, Ctxt &ctxt) const {
    ctxt[name] = expn->eval(defs, ctxt);
    return std::nullopt;
}

std::optional<Valu> Pass::exec([[maybe_unused]] const Defs& defs,
                               [[maybe_unused]] Ctxt& ctxt) const {
    // does nothing!
    return std::nullopt;
}
  
std::optional<Valu> Prnt::exec(const Defs& defs, Ctxt& ctxt) const {
    if (expns.size()) {
        std::cout << to_string(expns[0]->eval(defs,ctxt));
    }
    for (size_t i = 1; i < expns.size(); i++) {
        std::cout << " " << to_string(expns[i]->eval(defs,ctxt));
    }
    std::cout << std::endl;
    return std::nullopt;
}

std::optional<Valu> Pleq::exec(const Defs& defs, Ctxt& ctxt) const {
    Valu& val = ctxt[name];

    if (std::holds_alternative<int>(val)) {
        ctxt[name] = Valu(std::get<int>(val) + std::get<int>(expn->eval(defs,ctxt)));
    } else if (std::holds_alternative<std::string>(val)) {
        ctxt[name] = Valu(std::get<std::string>(val) + std::get<std::string>(expn->eval(defs,ctxt)));
    } else if (std::holds_alternative<bool>(val)) {
        ctxt[name] = Valu(std::get<bool>(val) + std::get<bool>(expn->eval(defs,ctxt)));
    }
    return std::nullopt;
}


std::optional<Valu> Mneq::exec(const Defs& defs, Ctxt& ctxt) const {
    Valu& val = ctxt[name];

    if (std::holds_alternative<int>(val)) {
        ctxt[name] = Valu(std::get<int>(val) - std::get<int>(expn->eval(defs,ctxt)));
    } else if (std::holds_alternative<std::string>(val)) {
        throw std::logic_error("Cannot subtract strings");
    } else if (std::holds_alternative<bool>(val)) {
        ctxt[name] = Valu(std::get<bool>(val) - std::get<bool>(expn->eval(defs,ctxt)));
    }
    return std::nullopt;
}


std::optional<Valu> Ifcd::exec([[maybe_unused]]const Defs& defs, [[maybe_unused]]Ctxt& ctxt) const {
    return std::nullopt;
}

std::optional<Valu> Cond::exec(const Defs& defs, Ctxt& ctxt) const {
    for (auto ifcd : ifcds) {
        if (std::get<bool>(ifcd->cond->eval(defs,ctxt))) {
            return ifcd->body->exec(defs,ctxt);
        }
    }
    if (els) {
        return els -> exec(defs, ctxt);
    } else {
        return std::nullopt;
    }
}


std::optional<Valu> Else::exec(const Defs& defs, Ctxt& ctxt) const {
    return body->exec(defs,ctxt);
}


std::optional<Valu> Elif::exec(const Defs& defs, Ctxt& ctxt) const {
    if (std::get<bool>(cond->eval(defs,ctxt))) {
        return cond->eval(defs,ctxt);
    } else {
        return body->exec(defs,ctxt);
    }
}


std::optional<Valu> Whil::exec(const Defs& defs, Ctxt& ctxt) const {
    while (std::get<bool>(cond->eval(defs,ctxt))) {
        body->exec(defs,ctxt);
    }
    return std::nullopt;
}


std::optional<Valu> Rept::exec(const Defs &defs, Ctxt &ctxt) const {
    do {
        body->exec(defs,ctxt);
    } while (!std::get<bool>(cond->eval(defs,ctxt)));
    return std::nullopt;
}


std::optional<Valu> FRtn::exec(const Defs& defs, Ctxt& ctxt) const {
    return expn->eval(defs,ctxt);
}

std::optional<Valu> PRtn::exec([[maybe_unused]]const Defs& defs, [[maybe_unused]]Ctxt& ctxt) const {
    return std::nullopt;
}



//
// Expn::eval
//
//  - evaluate DWISLPY expressions within a runtime context to determine their
//    (integer) value.
//

Valu Defn::exec(const Defs& defs, const Ctxt& ctxt, const Args_vec& vec) const {
    Ctxt new_ctxt { ctxt };
    if (args.get_frmls_size() != vec.size()) {
        throw std::logic_error("Wrong number of arguments");
    }

    for (size_t i = 0; i < args.get_frmls_size(); i++) {
        new_ctxt[args.get_frml(i)->name] = vec[i]->eval(defs, ctxt);
    }
    // handle return later 
    return body->exec(defs,new_ctxt).value_or(None);
}


Valu FCll::eval(const Defs& defs, const Ctxt& ctxt) const {
    auto fn_iter = defs.find(name);
    if (fn_iter == defs.end()) {
        throw std::logic_error("Function not found");
    }
    return fn_iter->second->exec(defs, ctxt, args);
}


std::optional<Valu> PCll::exec(const Defs& defs, Ctxt& ctxt) const {
    auto fn_iter = defs.find(name);
    if (fn_iter == defs.end()) {
        throw std::logic_error("Function not found");
    }
   fn_iter->second->exec(defs, ctxt, args);
   return std::nullopt;
}


Valu Inif::eval(const Defs& defs, const Ctxt& ctxt) const {
    if (std::get<bool>(cond->eval(defs, ctxt))) {
        return if_br->eval(defs, ctxt); 
    } else {
        return else_br->eval(defs, ctxt);
    }
}

Valu Negt::eval(const Defs& defs, const Ctxt& ctxt) const {
    return Valu(!std::get<bool>(expn->eval(defs,ctxt)));
}

Valu Imus::eval(const Defs& defs, const Ctxt& ctxt) const {
    return Valu(-std::get<int>(expn->eval(defs, ctxt)));
}


Valu Conj::eval(const Defs& defs, const Ctxt& ctxt) const {
    return Valu(std::get<bool>(lft->eval(defs,ctxt)) && std::get<bool>(rht->eval(defs,ctxt)));
};


Valu Disj::eval(const Defs& defs, const Ctxt& ctxt) const {
    return Valu(std::get<bool>(lft->eval(defs,ctxt)) || std::get<bool>(rht->eval(defs,ctxt)));
};


Valu Cmlt::eval(const Defs& defs, const Ctxt& ctxt) const {
    return Valu(std::get<int>(lft->eval(defs,ctxt)) < std::get<int>(rht->eval(defs,ctxt)));
};


Valu Cmgt::eval(const Defs& defs, const Ctxt& ctxt) const {
    return Valu(std::get<int>(lft->eval(defs,ctxt)) > std::get<int>(rht->eval(defs,ctxt)));
};


Valu Cmeq::eval(const Defs& defs, const Ctxt& ctxt) const {
    return Valu(std::get<int>(lft->eval(defs,ctxt)) == std::get<int>(rht->eval(defs,ctxt)));
};


Valu Cmle::eval(const Defs& defs, const Ctxt& ctxt) const {
    return Valu(std::get<int>(lft->eval(defs,ctxt)) <= std::get<int>(rht->eval(defs,ctxt)));
};


Valu Cmge::eval(const Defs& defs, const Ctxt& ctxt) const {
    return Valu(std::get<int>(lft->eval(defs,ctxt)) >= std::get<int>(rht->eval(defs,ctxt)));
};



Valu Plus::eval(const Defs& defs, const Ctxt& ctxt) const {
    Valu lv = left->eval(defs,ctxt);
    Valu rv = rght->eval(defs,ctxt);
    if (std::holds_alternative<int>(lv)
        && std::holds_alternative<int>(rv)) {
        int ln = std::get<int>(lv);
        int rn = std::get<int>(rv);
        return Valu {ln + rn};
    } else if (std::holds_alternative<std::string>(lv)
               && std::holds_alternative<std::string>(rv)) {
        std::string ls = std::get<std::string>(lv);
        std::string rs = std::get<std::string>(rv);
        return Valu {ls + rs};
    } else {
        std::string msg = "Run-time error: wrong operand type for plus.";
        throw DwislpyError { where(), msg };
    }        
}

Valu Mnus::eval(const Defs& defs, const Ctxt& ctxt) const {
    Valu lv = left->eval(defs,ctxt);
    Valu rv = rght->eval(defs,ctxt);
    if (std::holds_alternative<int>(lv)
        && std::holds_alternative<int>(rv)) {
        int ln = std::get<int>(lv);
        int rn = std::get<int>(rv);
        return Valu {ln - rn};
    } else {
        std::string msg = "Run-time error: wrong operand type for minus.";
        throw DwislpyError { where(), msg };
    }        
}

Valu Tmes::eval(const Defs& defs, const Ctxt& ctxt) const {
    Valu lv = left->eval(defs,ctxt);
    Valu rv = rght->eval(defs,ctxt);
    if (std::holds_alternative<int>(lv)
        && std::holds_alternative<int>(rv)) {
        int ln = std::get<int>(lv);
        int rn = std::get<int>(rv);
        return Valu {ln * rn};
    } else if (std::holds_alternative<std::string>(lv) && std::holds_alternative<int>(rv)) {
        std::string ln = std::get<std::string>(lv);
        int rn = std::get<int>(rv);
        std::stringstream ss; 
        for (unsigned int i = 0; i < rn; i++) {
            ss << ln;
        }
        return Valu {ss.str()};
    } 
    else {
        // Exercise: make this work for (int,str) and (str,int).
        std::string msg = "Run-time error: wrong operand type for times.";
        throw DwislpyError { where(), msg };
    }        
}

Valu IDiv::eval(const Defs& defs, const Ctxt& ctxt) const {
    Valu lv = left->eval(defs,ctxt);
    Valu rv = rght->eval(defs,ctxt);
    if (std::holds_alternative<int>(lv)
        && std::holds_alternative<int>(rv)) {
        int ln = std::get<int>(lv);
        int rn = std::get<int>(rv);
        if (rn == 0) {
            throw DwislpyError { where(), "Run-time error: division by 0."};
        } else {
            return Valu {ln / rn};
        } 
    } else {
        std::string msg = "Run-time error: wrong operand type for quotient.";
        throw DwislpyError { where(), msg };
    }        
}

Valu IMod::eval(const Defs& defs, const Ctxt& ctxt) const {
    Valu lv = left->eval(defs,ctxt);
    Valu rv = rght->eval(defs,ctxt);
    if (std::holds_alternative<int>(lv)
        && std::holds_alternative<int>(rv)) {
        int ln = std::get<int>(lv);
        int rn = std::get<int>(rv);
        if (rn == 0) {
            throw DwislpyError { where(), "Run-time error: division by 0."};
        } else {
            return Valu {ln % rn};
        } 
    } else {
        std::string msg = "Run-time error: wrong operand type for remainder.";
        throw DwislpyError { where(), msg };
    }        
}
Valu Ltrl::eval([[maybe_unused]] const Defs& defs,
                [[maybe_unused]] const Ctxt& ctxt) const {
    return valu;
}

Valu Lkup::eval([[maybe_unused]] const Defs& defs, const Ctxt& ctxt) const {
    if (ctxt.count(name) > 0) {
        return ctxt.at(name);
    } else {
        std::string msg = "Run-time error: variable '" + name +"'";
        msg += "not defined.";
        throw DwislpyError { where(), msg };
    }
}

Valu Inpt::eval([[maybe_unused]] const Defs& defs, const Ctxt& ctxt) const {
    Valu v = expn->eval(defs,ctxt);
    if (std::holds_alternative<std::string>(v)) {
        //
        std::string prompt = std::get<std::string>(v);
        std::cout << prompt;
        //
        std::string vl;
        std::cin >> vl;
        //
        return Valu {vl};
    } else {
        std::string msg = "Run-time error: prompt is not a string.";
        throw DwislpyError { where(), msg };
    }
}

Valu IntC::eval([[maybe_unused]] const Defs& defs, const Ctxt& ctxt) const {
    //
    // The integer conversion operation does nothing in this
    // version of DWISLPY.
    //
    Valu v = expn->eval(defs,ctxt);
    if (std::holds_alternative<int>(v)) {
        return Valu {v};
    } else if (std::holds_alternative<std::string>(v)) {
        std::string s = std::get<std::string>(v);
        try {
            int i = std::stoi(s);
            return Valu {i};
        } catch (std::invalid_argument e) {
            std::string msg = "Run-time error: \""+s+"\"";
            msg += "cannot be converted to an int.";
            throw DwislpyError { where(), msg };
        }
    } else if (std::holds_alternative<bool>(v)) {
        bool b = std::get<bool>(v);
        return Valu {b ? 1 : 0};
    } else {
        std::string msg = "Run-time error: cannot convert to an int.";
        throw DwislpyError { where(), msg };
    }
}

Valu StrC::eval([[maybe_unused]] const Defs& defs, const Ctxt& ctxt) const {
    //
    // The integer conversion operation does nothing in this
    // version of DWISLPY.
    //
    Valu v = expn->eval(defs,ctxt);
    return Valu { to_string(v) };
}

// * * * * *
//
// AST::output
//
// - Pretty printer for DWISLPY code represented in an AST.
//
// The code below is an implementation of a pretty printer. For each case
// of an AST node (each subclass) the `output` method provides the means for
// printing the code of the DWISLPY construct it represents.
//
//

void Pleq::output(std::ostream& os, std::string indent) const {
    os << indent << this->name << " += ";
    expn->output(os, indent);
    os << std::endl;
}

void Mneq::output(std::ostream& os, std::string indent) const {
    os << indent << this->name << " -= ";
    expn->output(os, indent);
    os << std::endl;

}

void Cond::output(std::ostream& os, std::string indent) const {
    this->ifcds[0]->output(os, indent);

    for (size_t index = 1; index < this->ifcds.size(); index++) {
        this->ifcds[index]->output(os, indent);
    }

    if (this->els) {
        this->els->output(os, indent);
    }
}

void Ifcd::output(std::ostream& os, std::string indent) const {
    os << indent << "if ";
    this->cond->output(os, indent);
    os << std::endl;
    this->body->output(os, indent + DEFAULT_INDENT_STR);
    os<<std::endl;
}

void Else::output(std::ostream& os, std::string indent) const {
    os << indent << "else" << std::endl;
    this->body->output(os, indent + DEFAULT_INDENT_STR);
    os<<std::endl;
}

void Elif::output(std::ostream& os, std::string indent) const {
    os << indent << "elif ";
    this->cond->output(os, indent);
    os << std::endl;
    this->body->output(os, indent + DEFAULT_INDENT_STR);
    os<<std::endl;

}

void Whil::output(std::ostream& os, std::string indent) const {
    os << indent << "while ";
    this->cond->output(os, indent);
    os << ": " << std::endl;
    this->body->output(os, indent + DEFAULT_INDENT_STR);
    os<<std::endl;
}

void Rept::output(std::ostream &os, std::string indent) const {
    os << indent << "repeat ";
    this->body->output(os, indent + DEFAULT_INDENT_STR);
    os << std::endl;
    this->cond->output(os, indent);
    os<<std::endl;
}

void FRtn::output(std::ostream& os, std::string indent) const {
    os << indent << "return ";
    this->expn->output(os, indent);
    os << std::endl;
}

void PRtn::output(std::ostream &os, std::string indent) const {
    os << indent << "return ";
    os << std::endl;
}

void Inif::output(std::ostream &os) const {
    this->if_br->output(os); 
    os << " if ";
    this->cond->output(os);
    os << " else ";
    this->else_br->output(os);
}

void Negt::output(std::ostream& os) const {
    os << "not ";
    this->expn->output(os);
}

void Imus::output(std::ostream& os) const {
    os << "-";
    this->expn->output(os);
}

void Conj::output(std::ostream& os) const {
    this->lft->output(os);
    os << " and ";
    this->rht->output(os);
}
    
void Disj::output(std::ostream& os) const {
    this->lft->output(os);
    os << " or ";
    this->rht->output(os);
}
    
void Cmlt::output(std::ostream& os) const {
    this->lft->output(os);
    os << " < ";
    this->rht->output(os);
}
    
void Cmgt::output(std::ostream& os) const {
    this->lft->output(os);
    os << " > ";
    this->rht->output(os);
}
    
void Cmeq::output(std::ostream& os) const {
    this->lft->output(os);
    os << " == ";
    this->rht->output(os);
}
    
void Cmle::output(std::ostream& os) const {
    this->lft->output(os);
    os << " <= ";
    this->rht->output(os);
}
    
void Cmge::output(std::ostream& os) const {
    this->lft->output(os);
    os << " >= ";
    this->rht->output(os);
}
    

void Prgm::output(std::ostream& os) const {
    for (auto defn : defs) {
        defn.second->output(os);
    }
    main->output(os);
    os << std::endl;
}

void Defn::output(std::ostream& os) const {
    this->output(os, "");
}

void dump_args(const Fmag& args, std::ostream& os) {
    if (args.get_frmls_size()) {
        auto arg = args.get_frml(0);
        os << arg->name << ": ";
        os << get_type_str(arg->type);
        for (size_t index = 1; index < args.get_frmls_size(); index++) {
            os << ", " << arg->name << ": ";
            os << get_type_str(arg->type);
        }
    }
}

void dump_args(const Args_vec& args, int level) {
    if (args.size()) {
        args[0]->dump(level);
        for (size_t index = 1; index < args.size(); index++) {
            args[index]->dump(level);
        }
    }
}

void output_args(const Args_vec& args, std::ostream& os) {
    if (args.size()) {
        args[0]->output(os);
        for (size_t index = 1; index < args.size(); index++) {
            os << ", ";
            args[index]->output(os);
        }
    }
}

void Defn::output(std::ostream& os, std::string indent) const {
    // Your code goes here.
    os << "def " << name << "(";
    dump_args(args, os);
    os << "): " << std::endl; // BOGUS to shut up compiler warning.
    body->output(os, indent + DEFAULT_INDENT_STR);
}

void PCll::output(std::ostream& os, std::string indent) const {
    os << indent <<  name << "(";
    output_args(args, os);
    os << ")";
}

void FCll::output(std::ostream& os) const {
    os << name << "(";
    output_args(args, os);
    os << ")";
}

void Blck::output(std::ostream& os, std::string indent) const {
    for (Stmt_ptr s : stmts) {
        s->output(os,indent);
    }
}

void Blck::output(std::ostream& os) const {
    for (Stmt_ptr s : stmts) {
        s->output(os, "");
    }
}

void Stmt::output(std::ostream& os) const {
    output(os,"");
}

void Ntro::output(std::ostream& os, std::string indent) const {
    os << indent << name << ": " << get_type_str(type) << " = ";
    this->expn->output(os, indent);
    os << std::endl;
}

void Asgn::output(std::ostream& os, std::string indent) const {
    os << indent;
    os << name << " = ";
    expn->output(os, indent);
    os << std::endl;
}

void Pass::output(std::ostream& os, std::string indent) const {
    os << indent << "pass" << std::endl;
}

void Prnt::output(std::ostream& os, std::string indent) const {
    os << indent;
    os << "print";
    os << "(";
    if (expns.size()) {
        expns[0]->output(os, indent);
    }
    for (size_t i = 1; i < expns.size(); i++)
        expns[i]->output(os, indent);
    os << ")";
    os << std::endl;
}

void Plus::output(std::ostream& os) const {
    os << "(";
    left->output(os);
    os << " + ";
    rght->output(os);
    os << ")";
}

void Mnus::output(std::ostream& os) const {
    os << "(";
    left->output(os);
    os << " - ";
    rght->output(os);
    os << ")";
}

void Tmes::output(std::ostream& os) const {
    os << "(";
    left->output(os);
    os << " * ";
    rght->output(os);
    os << ")";
}

void IDiv::output(std::ostream& os) const {
    os << "(";
    left->output(os);
    os << " // ";
    rght->output(os);
    os << ")";
}

void IMod::output(std::ostream& os) const {
    os << "(";
    left->output(os);
    os << " % ";
    rght->output(os);
    os << ")";
}

void Ltrl::output(std::ostream& os) const {
    os << to_repr(valu);
}

void Lkup::output(std::ostream& os) const {
    os << name;
}

void Inpt::output(std::ostream& os) const {
    os << "input(";
    expn->output(os);
    os << ")";
}

void IntC::output(std::ostream& os) const {
    os << "int(";
    expn->output(os);
    os << ")";
}

void StrC::output(std::ostream& os) const {
    os << "str(";
    expn->output(os);
    os << ")";
}


// * * * * *
//
// AST::dump
//
// - Dumps the AST of DWISLPY code.
//
// The code below dumps the contents of an AST as a series of lines
// indented and headed by the AST node type and its subcontents.
//

void dump_indent(int level) {
    //std::string indent { level, std::string {"    "} };
    for (int i=0; i<level; i++) {
        std::cout << "    ";
    }
}


void Ntro::dump(int level) const {
    dump_indent(level);
    std::cout << "Ntro: " << name << " : " << get_type_str(type) << std::endl;
    dump_indent(level + 1);
    expn->dump(level + 1);
}

void Pleq::dump(int level) const {
    dump_indent(level);
    std::cout << "Pleq" << std::endl;
    dump_indent(level + 1);
    std::cout << name << std::endl;
    expn->dump(level+1);
};

void Mneq::dump(int level) const {
    dump_indent(level);
    std::cout << "Pleq" << std::endl;
    dump_indent(level + 1);
    std::cout << name << std::endl;
    expn->dump(level+1);
};

void Cond::dump(int level) const {
    for (auto ifcd : ifcds) {
        ifcd->dump(level+1);
    }
    if (els) {
        els->dump(level+1);
    }
};

void Ifcd::dump(int level) const {
    dump_indent(level);
    std::cout << "If" << std::endl;
    cond->dump(level+1);
    body->dump(level+1);
};

void Else::dump(int level) const {
    dump_indent(level);
    std::cout << "Else" << std::endl;
    body->dump(level+1);
};


void Elif::dump(int level) const {
    dump_indent(level);
    std::cout << "Elif" << std::endl;
    cond->dump(level+1);
    body->dump(level+1);
};


void Whil::dump(int level) const {
    dump_indent(level);
    std::cout << "While" << std::endl;
    cond->dump(level+1);
    body->dump(level+1);
};

void Rept::dump(int level) const {
    dump_indent(level);
    std::cout << "Repeat" << std::endl;
    body->dump(level+1);
    std::cout << "Until" << std::endl;
    cond->dump(level+1);
};


void FRtn::dump(int level) const {
    dump_indent(level);
    std::cout << "Return" << std::endl;
    expn->dump(level+1);
};

void PRtn::dump(int level) const {
    dump_indent(level);
    std::cout << "Return" << std::endl;
};

void Inif::dump(int level) const {
    std::cout << "Inif" << std::endl;
    if_br->dump(level+1);
    cond->dump(level+1);
    else_br->dump(level+1);
}

void Negt::dump(int level) const {
    std::cout << "not ";
    expn->dump(level+1);
}

void Imus::dump(int level) const {
    std::cout << "-";
    expn->dump(level+1);
}

void Conj::dump(int level) const {
    lft->dump(level+1);
    std::cout << " and ";
    rht->dump(level+1);
}

void Disj::dump(int level) const {
    lft->dump(level+1);
    std::cout << " or ";
    rht->dump(level+1);
}

void Cmlt::dump(int level) const {
    lft->dump(level+1);
    std::cout << " < ";
    rht->dump(level+1);
}

void Cmgt::dump(int level) const {
    lft->dump(level+1);
    std::cout << " > ";
    rht->dump(level+1);
}

void Cmeq::dump(int level) const {
    lft->dump(level+1);
    std::cout << " == ";
    rht->dump(level+1);
}

void Cmle::dump(int level) const {
    lft->dump(level+1);
    std::cout << " <= ";
    rht->dump(level+1);
}

void Cmge::dump(int level) const {
    lft->dump(level+1);
    std::cout << " >= ";
    rht->dump(level+1);
}



void Prgm::dump(int level) const {
    dump_indent(level);
    std::cout << "PRGM" << std::endl;
    for (auto defn : defs) {
        defn.second->dump(level+1);
    }

    if (main) {
        main->dump(level+1);
    }

    std::cout << std::endl;
}

void Defn::dump(int level) const {
    dump_indent(level);
    std::cout << "DEFN" << std::endl;
    // Your code goes here.
    dump_indent(level+1);
    std::cout << "Args" << std::endl;
    dump_indent(level + 2);
    std::cout << name << std::endl;
    dump_indent(level + 1);
    dump_args(args, std::cout);
    std::cout << std::endl;
    body->dump(level+1);
}

void PCll::dump(int level) const {
    dump_indent(level);
    std::cout << "PCALL" << std::endl;
    dump_indent(level+1);
    std::cout << name << std::endl;
    dump_args(args, level + 1);
    std::cout << std::endl;
}

void FCll::dump(int level) const {
    dump_indent(level);
    std::cout << "FCALL" << std::endl;
    dump_indent(level+1);
    std::cout << name << std::endl;
    dump_args(args, level + 1);
    std::cout << std::endl;
}

void Blck::dump(int level) const {
    dump_indent(level);
    std::cout << "BLCK" << std::endl;
    for (Stmt_ptr stmt : stmts) {
        stmt->dump(level+1);
    }
}

void Asgn::dump(int level) const {
    dump_indent(level);
    std::cout << "ASGN" << std::endl;
    dump_indent(level+1);
    std::cout << name << std::endl;
    expn->dump(level+1);
}

void Prnt::dump(int level) const {
    dump_indent(level);
    std::cout << "PRNT" << std::endl;
    for (auto expn : expns) {
        expn->dump(level+1);
    }
}

void Pass::dump(int level) const {
    dump_indent(level);
    std::cout << "PASS" << std::endl;
}

void Plus::dump(int level) const {
    dump_indent(level);
    std::cout << "PLUS" << std::endl;
    left->dump(level+1);
    rght->dump(level+1);
}

void Mnus::dump(int level) const {
    dump_indent(level);
    std::cout << "MNUS" << std::endl;
    left->dump(level+1);
    rght->dump(level+1);
}

void Tmes::dump(int level) const {
    dump_indent(level);
    std::cout << "TMES" << std::endl;
    left->dump(level+1);
    rght->dump(level+1);
}

void IDiv::dump(int level) const {
    dump_indent(level);
    std::cout << "IDIV" << std::endl;
    left->dump(level+1);
    rght->dump(level+1);
}

void IMod::dump(int level) const {
    dump_indent(level);
    std::cout << "IDIV" << std::endl;
    left->dump(level+1);
    rght->dump(level+1);
}

void Ltrl::dump(int level) const {
    dump_indent(level);
    std::cout << "LTRL" << std::endl;
    dump_indent(level+1);
    std::cout << to_repr(valu) << std::endl;
}

void Lkup::dump(int level) const {
    dump_indent(level);
    std::cout << "LKUP" << std::endl;
    dump_indent(level+1);
    std::cout << name << std::endl;
}

void Inpt::dump(int level) const {
    dump_indent(level);
    std::cout << "INPT" << std::endl;
    expn->dump(level+1);
}

void IntC::dump(int level) const {
    dump_indent(level);
    std::cout << "INTC" << std::endl;
    expn->dump(level+1);
}

void StrC::dump(int level) const {
    dump_indent(level);
    std::cout << "STRC" << std::endl;
    expn->dump(level+1);
}

