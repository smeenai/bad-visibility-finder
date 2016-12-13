#include <unordered_map>
#include <vector>

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
    if (!Decl->isThisDeclarationADefinition())
      return true;

    if (Decl->getTemplatedKind() == FunctionDecl::TK_NonTemplate)
      return true;

    if (Decl->isInlineSpecified())
      return true;

    if (Decl->getExplicitVisibility(NamedDecl::VisibilityForValue)
            .getValueOr(DefaultVisibility) == HiddenVisibility)
      return true;

    CXXRecordDecl *Class = Decl->getParent()->getCanonicalDecl();
    if (Class->getExplicitVisibility(NamedDecl::VisibilityForValue)
            .getValueOr(HiddenVisibility) != HiddenVisibility) {
      printBadMethod(Decl);
      return true;
    }

    ClassTemplateDecl *Template = Class->getDescribedClassTemplate();
    if (Template == nullptr)
      return true;

    auto Specialization = DefaultVisibilityInstantiations.find(Class);
    if (Specialization == DefaultVisibilityInstantiations.end()) {
      BadVisibilityMethods[Class].push_back(Decl);
      return true;
    }

    printBadMethod(Decl, Specialization->second);
    return true;
  }

  bool
  VisitClassTemplateSpecializationDecl(ClassTemplateSpecializationDecl *Decl) {
    // TODO: handle other types of specializations
    if (Decl->getTemplateSpecializationKind() !=
        TSK_ExplicitInstantiationDeclaration)
      return true;

    if (Decl->getExplicitVisibility(NamedDecl::VisibilityForValue)
            .getValueOr(HiddenVisibility) == HiddenVisibility)
      return true;

    CXXRecordDecl *Class =
        Decl->getSpecializedTemplate()->getTemplatedDecl()->getCanonicalDecl();
    if (DefaultVisibilityInstantiations.count(Class) == 1)
      return true;

    DefaultVisibilityInstantiations.emplace(Class, Decl);
    for (auto *Method : BadVisibilityMethods[Class])
      printBadMethod(Method, Decl);
    return true;
  }

private:
  ASTContext &Context;
  std::unordered_map<CXXRecordDecl *, ClassTemplateSpecializationDecl *>
      DefaultVisibilityInstantiations;
  std::unordered_map<CXXRecordDecl *, std::vector<CXXMethodDecl *>>
      BadVisibilityMethods;

  void printBadMethod(
      const CXXMethodDecl *Decl,
      const ClassTemplateSpecializationDecl *Specialization = nullptr) {
    llvm::outs() << getSourceLoc(Decl) << " ";
    Decl->printQualifiedName(llvm::outs());
    if (Specialization != nullptr)
      llvm::outs() << " (from specialization at "
                   << getSourceLoc(Specialization) << ")";
    llvm::outs() << "\n";
  }

  std::string getSourceLoc(const Decl *Decl) {
    FullSourceLoc FullLoc =
        Context.getFullLoc(Decl->getLocStart()).getExpansionLoc();
    return (Context.getSourceManager().getFilename(FullLoc) + ":" +
            llvm::Twine(FullLoc.getExpansionLineNumber()))
        .str();
  }
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
