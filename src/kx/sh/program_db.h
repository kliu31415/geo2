#pragma once

#include <memory>
#include <string>
#include <mutex>
#include <map>

namespace kx { namespace sh {

class KShProgramDb
{
    class Abstract_KSP_CtorHolder
    {
        friend class KShProgramDb;
        virtual std::unique_ptr<KShellProgram> construct(std::shared_ptr<KShell> shell) = 0;
    public:
        virtual ~Abstract_KSP_CtorHolder() {} ///virtual dtor just in case we add fields later
    };
    //This "holds" a constructor for T, where T is a KShellProgram
    template<class T> class KSP_CtorHolder final: public Abstract_KSP_CtorHolder
    {
        std::unique_ptr<KShellProgram> construct(std::shared_ptr<KShell> shell) override
        {
            return std::make_unique<T>(std::move(shell));
        }
    };

    std::map<std::string, std::unique_ptr<Abstract_KSP_CtorHolder>> programs;
    std::mutex mtx;
public:
    KShProgramDb() = default;

    ///There's no reason to copy this, so disable it for safety.
    KShProgramDb(const KShProgramDb&) = delete;
    KShProgramDb &operator = (const KShProgramDb&) = delete;

    ///nonmovable because KShell keeps a pointer to it
    KShProgramDb(KShProgramDb&&) = delete;
    KShProgramDb &operator = (KShProgramDb&&) = delete;

    std::vector<std::string> get_program_list()
    {
        std::lock_guard<std::mutex> lg(mtx);
        std::vector<std::string> program_list;
        program_list.resize(programs.size());
        size_t cnt = 0;
        for(const auto &program: programs)
            program_list[cnt++] = program.first;
        return program_list;
    }
    std::unique_ptr<KShellProgram> construct_program(std::shared_ptr<KShell> shell,
                                                     const std::string &name)
    {
        std::lock_guard<std::mutex> lg(mtx);
        auto prog_iter = programs.find(name);
        if(prog_iter != programs.end()) {
            return prog_iter->second->construct(std::move(shell));
        } else
            return nullptr;
    }
    template<class Program> void add_program_nonatomic(std::string name)
    {
        k_expects(programs.find(name) == programs.end());
        programs[std::move(name)] = std::make_unique<KSP_CtorHolder<Program>>();
    }
    template<class Program> void add_program(std::string name)
    {
        k_expects(programs.find(name) == programs.end());
        std::lock_guard<std::mutex> lg(mtx);
        programs[std::move(name)] = std::make_unique<KSP_CtorHolder<Program>>();
    }
};

}}
