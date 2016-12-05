#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;
using namespace clang::tooling;

static llvm::cl::OptionCategory
    BadVisibilityFinderCategory("Bad visibility finder options");

class BadVisibilityFinderVisitor
    : public RecursiveASTVisitor<BadVisibilityFinderVisitor> {
public:
  explicit BadVisibilityFinderVisitor(ASTContext &Context) : Context(Context) {}

  bool VisitCXXMethodDecl(CXXMethodDecl *Decl) {
    if (!Decl->isThisDeclarationADefinition()) {
      return true;
    }

    if (Decl->getTemplatedKind() == FunctionDecl::TK_NonTemplate) {
      return true;
    }

    if (Decl->isInlineSpecified()) {
      return true;
    }

    if (Decl->getExplicitVisibility(NamedDecl::VisibilityForValue).hasValue()) {
      return true;
    }

    if (Decl->getParent()
            ->getExplicitVisibility(NamedDecl::VisibilityForValue)
            .getValueOr(HiddenVisibility) != DefaultVisibility) {
      return true;
    }

    FullSourceLoc FullLoc = Context.getFullLoc(Decl->getLocStart());
    assert(FullLoc.isValid());
    llvm::outs() << Context.getSourceManager().getFilename(FullLoc) << ":"
                 << FullLoc.getSpellingLineNumber() << " ";
    Decl->printQualifiedName(llvm::outs());
    llvm::outs() << "\n";
    return true;
  }

private:
  ASTContext &Context;
};

class BadVisibilityFinderConsumer : public ASTConsumer {
public:
  explicit BadVisibilityFinderConsumer(ASTContext &Context)
      : Visitor(Context) {}

  void HandleTranslationUnit(ASTContext &Context) override {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }

private:
  BadVisibilityFinderVisitor Visitor;
};

class BadVisibilityFinderAction : public ASTFrontendAction {
public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {
    return llvm::make_unique<BadVisibilityFinderConsumer>(CI.getASTContext());
  }
};

int main(int argc, char **argv) {
  CommonOptionsParser op(argc, const_cast<const char **>(argv),
                         BadVisibilityFinderCategory);
  ClangTool Tool(op.getCompilations(), op.getSourcePathList());
  return Tool.run(newFrontendActionFactory<BadVisibilityFinderAction>().get());
}
