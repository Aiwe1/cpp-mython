#include "runtime.h"

#include <cassert>
#include <optional>
#include <sstream>

using namespace std;

namespace runtime {

    ObjectHolder::ObjectHolder(std::shared_ptr<Object> data)
        : data_(std::move(data)) {
    }

    void ObjectHolder::AssertIsValid() const {
        assert(data_ != nullptr);
    }

    ObjectHolder ObjectHolder::Share(Object& object) {
        // Возвращаем невладеющий shared_ptr (его deleter ничего не делает)
        return ObjectHolder(std::shared_ptr<Object>(&object, [](auto* /*p*/) { /* do nothing */ }));
    }

    ObjectHolder ObjectHolder::None() {
        return ObjectHolder();
    }

    Object& ObjectHolder::operator*() const {
        AssertIsValid();
        return *Get();
    }

    Object* ObjectHolder::operator->() const {
        AssertIsValid();
        return Get();
    }

    Object* ObjectHolder::Get() const {
        return data_.get();
    }

    ObjectHolder::operator bool() const {
        return Get() != nullptr;
    }

    bool IsTrue(const ObjectHolder& object) {
        {
            auto t = object.TryAs<Bool>();
            if (t) {
                if (!t->GetValue())
                    return false;
                return true;
            }
        }
        {
            auto t = object.TryAs<Number>();
            if (t) {
                if (t->GetValue() == 0)
                    return false;
                return true;
            }
        }
        {
            auto t = object.TryAs<String>();
            if (t) {
                if (t->GetValue().empty())
                    return false;
                return true;
            }
        }
        return false;
    }

    void ClassInstance::Print(std::ostream& os, Context& context) {
        //
        if (HasMethod("__str__", 0)) {
            //cls_.GetMethod("__str__")->body->Execute(closure_, context);
            auto res = Call("__str__", {}, context);
            res.Get()->Print(os, context);
        }
        else {
            os << this;
        }

    }

    bool ClassInstance::HasMethod(const std::string& method, size_t argument_count) const {
        // Заглушка, реализуйте метод самостоятельно
        auto m = cls_.GetMethod(method);
        if (m && m->formal_params.size() == argument_count) {
            return true;
        }
        return false;
    }

    Closure& ClassInstance::Fields() {
        return closure_;
    }

    const Closure& ClassInstance::Fields() const {
        return closure_;
    }

    ClassInstance::ClassInstance(const Class& cls) : cls_(cls) {
    }

    ObjectHolder ClassInstance::Call(const std::string& method,
                const std::vector<ObjectHolder>& actual_args, Context& context) {

        if (HasMethod(method, actual_args.size())) {
            Closure args;
            args["self"s] = ObjectHolder::Share(*this);

            auto m = cls_.GetMethod(method);

            for (size_t i = 0; i < actual_args.size(); ++i) {
                args[m->formal_params[i]] = actual_args[i];
            }
            return m->body->Execute(args, context);
        }
        else {
            throw std::runtime_error("Not implemented"s);
        }
    }

    Class::Class(std::string name, std::vector<Method> methods, const Class* parent)
        :  name_(name), methods_(move(methods)), parent_(parent) {    
    }

    const Method* Class::GetMethod(const std::string& name) const {
        for (auto p = this; p != nullptr; p = p->parent_) {            
            for (const auto& m : p->methods_) {
                if (m.name == name) {
                    return &m;
                }
            }
        }
        // Заглушка. Реализуйте метод самостоятельно
        return nullptr;
    }

    [[nodiscard]] const std::string& Class::GetName() const {
        // Заглушка. Реализуйте метод самостоятельно.
        //throw std::runtime_error("Not implemented"s);
        return name_;
    }

    void Class::Print(ostream& os, [[maybe_unused]] Context& context) {
        // Заглушка. Реализуйте метод самостоятельно
        os << "Class " << name_;
    }

    void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context) {
        os << (GetValue() ? "True"sv : "False"sv);
    }

    bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        // Заглушка. Реализуйте функцию самостоятельно
        if (lhs.Get() == nullptr && rhs.Get() == nullptr) {
            return true;
        }
        if (lhs.Get() == nullptr || rhs.Get() == nullptr) {
            throw std::runtime_error("Cannot compare objects for equality"s);
        }

        {
            auto t1 = lhs.TryAs<Bool>();
            auto t2 = rhs.TryAs<Bool>();
            if (t1 && t2) {
                if (t1->GetValue() == t2->GetValue()) {
                    return true;
                }
                return false;
            }
        }
        {
            auto t1 = lhs.TryAs<String>();
            auto t2 = rhs.TryAs<String>();
            if (t1 && t2) {
                if (t1->GetValue() == t2->GetValue()) {
                    return true;
                }
                return false;
            }
        }
        {
            auto t1 = lhs.TryAs<Number>();
            auto t2 = rhs.TryAs<Number>();
            if (t1 && t2) {
                if (t1->GetValue() == t2->GetValue()) {
                    return true;
                }
                return false;
            }
        }
        {
            auto t1 = lhs.TryAs<ClassInstance>();
            auto t2 = rhs.TryAs<ClassInstance>();
            if (t1 && t2) {
                if (t1->HasMethod("__eq__", 1)) {
                    vector<ObjectHolder> v;
                    v.push_back(rhs);
                    //auto asda = t1->Call("__eq__", v, context).TryAs<Bool>();
                    return t1->Call("__eq__", v, context).TryAs<Bool>()->GetValue();
                }
            }
        }
        throw std::runtime_error("Cannot compare objects for equality"s);
    }

    /*
     * Если lhs и rhs - числа, строки или значения bool, функция возвращает результат их сравнения
     * оператором <.
     * Если lhs - объект с методом __lt__, возвращает результат вызова lhs.__lt__(rhs),
     * приведённый к типу bool. В остальных случаях функция выбрасывает исключение runtime_error.
     *
     * Параметр context задаёт контекст для выполнения метода __lt__
     */
    bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        // Заглушка. Реализуйте функцию самостоятельно
        //if (lhs.Get() == nullptr && rhs.Get() == nullptr) {
        //    return true;
        //}
        if (lhs.Get() == nullptr || rhs.Get() == nullptr) {
            throw std::runtime_error("Cannot compare objects for equality"s);
        }

        {
            auto t1 = lhs.TryAs<Bool>();
            auto t2 = rhs.TryAs<Bool>();
            if (t1 && t2) {
                return t1->GetValue() < t2->GetValue();
            }
        }
        {
            auto t1 = lhs.TryAs<String>();
            auto t2 = rhs.TryAs<String>();
            if (t1 && t2) {
                return t1->GetValue() < t2->GetValue();
            }
        }
        {
            auto t1 = lhs.TryAs<Number>();
            auto t2 = rhs.TryAs<Number>();
            if (t1 && t2) {
                return t1->GetValue() < t2->GetValue();
            }
        }
        {
            auto t1 = lhs.TryAs<ClassInstance>();
            auto t2 = rhs.TryAs<ClassInstance>();
            if (t1 && t2) {
                if (t1->HasMethod("__lt__", 1)) {
                    vector<ObjectHolder> v;
                    v.push_back(rhs);
                    return t1->Call("__lt__", v, context).TryAs<Bool>()->GetValue();
                }
            }
        }
        throw std::runtime_error("Cannot compare objects for equality"s);
        return false;
        //throw std::runtime_error("Cannot compare objects for less"s);
    }

    bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        // Заглушка. Реализуйте функцию самостоятельно
        return !Equal(lhs, rhs, context);
        //throw std::runtime_error("Cannot compare objects for equality"s);
    }

    bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        // Заглушка. Реализуйте функцию самостоятельно
        return (!Less(lhs, rhs, context) && !Equal(lhs, rhs, context));
        //throw std::runtime_error("Cannot compare objects for equality"s);
    }

    bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        // Заглушка. Реализуйте функцию самостоятельно
        return (Less(lhs, rhs, context) || Equal(lhs, rhs, context));
    }

    bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        // Заглушка. Реализуйте функцию самостоятельно
        return !Less(lhs, rhs, context);
    }

}  // namespace runtime
