#include <string>
#include <clang/AST/ASTContext.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclGroup.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Parse/ParseAST.h>
#include <clang/Lex/Preprocessor.h>

struct consumer : clang::ASTConsumer {
    explicit consumer(std::string const& target)
    : target_(target),
      ctx_(nullptr) {}

    void HandleTopLevelDecl(clang::DeclGroupRef decls) {
        for (auto& decl : decls) {
            llvm::errs() << ".\n";
            if (auto const* fd = llvm::dyn_cast<clang::FunctionDecl>(decl)) {
                handle_functiondecl(fd);
            }
            if (llvm::dyn_cast<clang::NamespaceDecl>(decl)) {
                llvm::errs() << "NS\n";
            }
        }
    }

    void Initialize(clang::ASTContext& ctx) {
        ctx_ = &ctx;
    }

private:
    void handle_functiondecl(clang::FunctionDecl const* fd) const {
        llvm::errs() << fd->getDeclName().getAsString() << '\n';
        if (fd->getDeclName().getAsString() == target_) {
            auto const& range = fd->getSourceRange();
            range.getBegin().print(llvm::outs(), ctx_->getSourceManager());
            llvm::outs() << '\n';
        }
    }

    std::string target_;
    clang::ASTContext* ctx_;
};

int main(int argc, char** argv) {
    clang::CompilerInstance compiler;
    compiler.createDiagnostics(argc, argv);

    auto& diag = compiler.getDiagnostics();
    auto& invocation = compiler.getInvocation();

    clang::CompilerInvocation::CreateFromArgs(invocation, argv + 1, argv + argc - 1, diag);
    compiler.setTarget(clang::TargetInfo::CreateTargetInfo(diag, compiler.getTargetOpts()));

    compiler.createFileManager();
    compiler.createSourceManager(compiler.getFileManager());
    compiler.createPreprocessor();
    compiler.createASTContext();
    compiler.setASTConsumer(new consumer(argv[argc - 1]));
    compiler.createSema(false, nullptr);

    auto& pp = compiler.getPreprocessor();
    pp.getBuiltinInfo().InitializeBuiltins(
        pp.getIdentifierTable(), pp.getLangOptions());

    auto& inputs = compiler.getFrontendOpts().Inputs;
    if (inputs.size() > 0) {
        compiler.InitializeSourceManager(inputs[0].second);
        clang::ParseAST(
            pp,
            &compiler.getASTConsumer(),
            compiler.getASTContext()
        );
    }

    return 0;
}
