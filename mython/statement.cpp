#include "statement.h"

#include <iostream>
#include <sstream>

using namespace std;

namespace ast {

    using runtime::Closure;
    using runtime::Context;
    using runtime::ObjectHolder;

    namespace {
        const string ADD_METHOD = "__add__"s;
        const string INIT_METHOD = "__init__"s;
    }  // namespace

    ObjectHolder Assignment::Execute(Closure& closure, Context& context) {
        // Заглушка. Реализуйте метод самостоятельно
        if (var_.size() > 0) {
            closure[var_] = rv_->Execute(closure, context);
            return closure.at(var_);
        }
        throw std::runtime_error("Wrong arg");
    }

    Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv)
        : var_(std::move(var)), rv_(std::move(rv)) {
    }

    VariableValue::VariableValue(const std::string& var_name) {
        dotted_ids_.push_back(var_name);
    }

    VariableValue::VariableValue(std::vector<std::string> dotted_ids) : dotted_ids_(std::move(dotted_ids)) {
    }

    ObjectHolder VariableValue::Execute(Closure& closure, Context& ) {
        // Заглушка. Реализуйте метод самостоятельно
        if (const auto it = closure.find(dotted_ids_[0]); it != closure.end()) {
            ObjectHolder obj = it->second;

            if (dotted_ids_.size() > 1) {
                for (size_t i = 1; i < dotted_ids_.size() - 1; ++i) {
                    if (auto* class_ptr = obj.TryAs<runtime::ClassInstance>()) {
                        if (const auto item = class_ptr->Fields().find(dotted_ids_[i]);
                            item != class_ptr->Fields().end()) {
                            obj = item->second;
                            continue;
                        }
                    }
                    throw std::runtime_error("Wrong arg"s);
                }
                const auto& fields = obj.TryAs<runtime::ClassInstance>()->Fields();
                if (const auto it = fields.find(dotted_ids_.back()); it != fields.end()) {
                    return it->second;
                }

            }
            else {
                return obj;
            }
        }
        throw std::runtime_error("Wrong arg");
    }

    unique_ptr<Print> Print::Variable(const std::string& name) {
        return std::make_unique<Print>(std::make_unique<VariableValue>(name));
    }

    Print::Print(unique_ptr<Statement> argument) {
        args_.push_back(std::move(argument));
    }

    Print::Print(vector<unique_ptr<Statement>> args) : args_(std::move(args)) {
        // Заглушка, реализуйте метод самостоятельно
    }

    // Во время выполнения команды print вывод должен осуществляться в поток, возвращаемый из
    // context.GetOutputStream()
    ObjectHolder Print::Execute(Closure& closure, Context& context) {
        bool flag = false;
        for (const auto& arg : args_) {
            auto value = arg->Execute(closure, context);
            if (flag) {
                context.GetOutputStream() << ' ';
            }

            if (value) {
                value->Print(context.GetOutputStream(), context);
            }
            else {
                context.GetOutputStream() << "None"s;
            }
            flag = true;
        }
        context.GetOutputStream() << '\n';

        return ObjectHolder::None();
    }

    MethodCall::MethodCall(std::unique_ptr<Statement> object, std::string method,
        std::vector<std::unique_ptr<Statement>> args) : object_(std::move(object)), method_(std::move(method)),
        args_(std::move(args)) {
        // Заглушка. Реализуйте метод самостоятельно
    }

    ObjectHolder MethodCall::Execute(Closure& closure, Context& context) {
        std::vector<runtime::ObjectHolder> args;
        for (const auto& arg : args_) {
            args.push_back(arg->Execute(closure, context));
        }

        auto* cls = object_->Execute(closure, context).TryAs<runtime::ClassInstance>();
        if (!cls) {
            throw std::runtime_error("Cannot find class"s);
        }

        return cls->Call(method_, args, context);
    }

    ObjectHolder Stringify::Execute(Closure& closure, Context& context) {
        // Заглушка. Реализуйте метод самостоятельно
        stringstream ss;
        
        auto a = argument_.get()->Execute(closure, context); //.Get()->Print(ss, context);
        if (a) {
            a.Get()->Print(ss, context);
        }
        else {
            ss << "None";
        }
        string s;
        s = ss.str();

        return ObjectHolder::Own(runtime::ValueObject<string>(s));
        return{};
    }

    ObjectHolder Add::Execute(Closure& closure, Context& context) {
        // Заглушка. Реализуйте метод самостоятельно
        auto lhs = lhs_->Execute(closure, context);
        auto rhs = rhs_->Execute(closure, context);

        {
            auto n1 = lhs.TryAs<runtime::Number>();
            auto n2 = rhs.TryAs<runtime::Number>();
            if (n1 && n2) {
                return runtime::ObjectHolder::Own<runtime::Number>(n1->GetValue() + n2->GetValue());
            }
        }
        {
            auto n1 = lhs.TryAs<runtime::String>();
            auto n2 = rhs.TryAs<runtime::String>();
            if (n1 && n2) {
                return runtime::ObjectHolder::Own<runtime::String>(n1->GetValue() + n2->GetValue());
            }
        }
        {
            auto n1 = lhs.TryAs<runtime::ClassInstance>();
            //auto n2 = rhs.TryAs<runtime::ClassInstance>();
            if (n1) {
                if (n1->HasMethod("__add__", 1)) {
                    vector<ObjectHolder> v;
                    v.push_back(rhs);
                    return n1->Call("__add__", v, context);
                }
            }
        }

        throw std::runtime_error("Ne to");
    }

    ObjectHolder Sub::Execute(Closure& closure, Context& context) {
        auto lhs = lhs_->Execute(closure, context);
        auto rhs = rhs_->Execute(closure, context);

        if (lhs && rhs) {
            auto n1 = lhs.TryAs<runtime::Number>();
            auto n2 = rhs.TryAs<runtime::Number>();

            return ObjectHolder::Own(runtime::Number(n1->GetValue() - n2->GetValue()));
        }
        
        throw std::runtime_error("Sub wrong"s);
    }

    ObjectHolder Mult::Execute(Closure& closure, Context& context) {
        auto lhs = lhs_->Execute(closure, context);
        auto rhs = rhs_->Execute(closure, context);

        if (lhs && rhs) {
            auto n1 = lhs.TryAs<runtime::Number>();
            auto n2 = rhs.TryAs<runtime::Number>();

            return ObjectHolder::Own(runtime::Number(n1->GetValue() * n2->GetValue()));
        }

        throw std::runtime_error("Mult wrong"s);
    }

    ObjectHolder Div::Execute(Closure& closure, Context& context) {
        auto lhs = lhs_->Execute(closure, context);
        auto rhs = rhs_->Execute(closure, context);

        if (lhs && rhs) {
            auto n1 = lhs.TryAs<runtime::Number>();
            auto n2 = rhs.TryAs<runtime::Number>();
            if (n2->GetValue() == 0) {
                throw std::runtime_error("Div na 0"s);
            }
            return ObjectHolder::Own(runtime::Number(n1->GetValue() / n2->GetValue()));
        }

        throw std::runtime_error("Div wrong"s);
    }

    ObjectHolder Compound::Execute(Closure& closure, Context& context) {
        for (auto& a : args_) {
            a->Execute(closure, context);
        }

        return {};
    }

    ObjectHolder Return::Execute(Closure& closure, Context& context) {
        throw statement_->Execute(closure, context);
    }

    ClassDefinition::ClassDefinition(ObjectHolder cls) : class_(std::move(cls)) {
        // Заглушка. Реализуйте метод самостоятельно
    }

    ObjectHolder ClassDefinition::Execute(Closure& closure, Context& ) {
        closure[class_.TryAs<runtime::Class>()->GetName()] = class_;
        return class_;
    }

    FieldAssignment::FieldAssignment(VariableValue object, std::string field_name,
        std::unique_ptr<Statement> rv) : object_(std::move(object)), field_name_(std::move(field_name))
        , rv_(std::move(rv)) {
    }

    // Присваивает полю object.field_name значение выражения rv
    ObjectHolder FieldAssignment::Execute(Closure& closure, Context& context) {
        auto* cls = object_.Execute(closure, context).TryAs<runtime::ClassInstance>();
        if (!cls) {
            throw std::runtime_error("no class"s);
        }
        cls->Fields()[field_name_] = rv_->Execute(closure, context);

        return cls->Fields()[field_name_];
    }

    IfElse::IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
        std::unique_ptr<Statement> else_body) : condition_(std::move(condition)), if_body_(std::move(if_body)),
        else_body_(std::move(else_body)) {
        // Реализуйте метод самостоятельно
    }

    ObjectHolder IfElse::Execute(Closure& closure, Context& context) {
        if (runtime::IsTrue(condition_->Execute(closure, context))) {
            return if_body_->Execute(closure, context);
        }
        if (else_body_) {
            return else_body_->Execute(closure, context);
        }

        return {};
    }

    ObjectHolder Or::Execute(Closure& closure, Context& context) {
        // Заглушка. Реализуйте метод самостоятельно
        auto lhs = lhs_.get()->Execute(closure, context);
        if (!lhs.TryAs<runtime::Bool>()->GetValue()) {
            auto rhs = rhs_.get()->Execute(closure, context);
            return ObjectHolder::Own<runtime::Bool>(rhs.TryAs<runtime::Bool>()->GetValue());
        }
        return ObjectHolder::Own<runtime::Bool>(true);

    }

    ObjectHolder And::Execute(Closure& closure, Context& context) {
        auto lhs = lhs_.get()->Execute(closure, context);

        if (lhs.TryAs<runtime::Bool>()->GetValue()) {
            auto rhs = rhs_.get()->Execute(closure, context);
            return ObjectHolder::Own<runtime::Bool>(rhs.TryAs<runtime::Bool>()->GetValue());
        }
        return ObjectHolder::Own<runtime::Bool>(false);
    }

    ObjectHolder Not::Execute(Closure& closure, Context& context) {
        auto arg = argument_.get()->Execute(closure, context);

        return ObjectHolder::Own<runtime::Bool>(!(arg.TryAs<runtime::Bool>()->GetValue()));
    }

    Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
        : BinaryOperation(std::move(lhs), std::move(rhs)), cmp_(std::move(cmp)) {
        // Реализуйте метод самостоятельно
    }

    ObjectHolder Comparison::Execute(Closure& closure, Context& context) {
        return ObjectHolder::Own(runtime::Bool(cmp_(lhs_->Execute(closure, context), 
            rhs_->Execute(closure, context), context)));
    }

    NewInstance::NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args) 
        : class_(class_), args_(std::move(args)) {
    }

    NewInstance::NewInstance(const runtime::Class& class_) : class_(class_) {   
    }

    ObjectHolder NewInstance::Execute(Closure& closure, Context& context) {
        ObjectHolder obj = ObjectHolder::Own(runtime::ClassInstance(class_));
        auto new_instance = obj.TryAs<runtime::ClassInstance>();
        if (new_instance && new_instance->HasMethod("__init__", args_.size())) {
            std::vector<runtime::ObjectHolder> new_args;
            for (const auto& arg : args_) {
                new_args.push_back(arg->Execute(closure, context));
            }
            new_instance->Call("__init__", new_args, context);
        }
        return obj;
    }

    MethodBody::MethodBody(std::unique_ptr<Statement>&& body) : body_(std::move(body)) {
    }

    ObjectHolder MethodBody::Execute(Closure& closure, Context& context) {
        ObjectHolder res = ObjectHolder::None();

        try {
            res = body_->Execute(closure, context);
        }
        catch (ObjectHolder& obj) { res = std::move(obj); }

        return res;
    }

}  // namespace ast
