// $Id: control.cpp,v 1.88 2006/03/15 06:19:48 shini Exp $
//
// This software is subject to the terms of the IBM Jikes Compiler
// License Agreement available at the following URL:
// http://ibm.com/developerworks/opensource/jikes.
// Copyright (C) 1996, 2004 IBM Corporation and others.  All Rights Reserved.
// You must accept the terms of that agreement to use this software.
//
#include "control.h"
#include "scanner.h"
#include "parser.h"
#include "semantic.h"
#include "error.h"
#include "bytecode.h"
#include "case.h"
#include "option.h"
#include <fstream>
#include <dlfcn.h>
#include <cstdlib>

#ifdef HAVE_JIKES_NAMESPACE
namespace Jikes { // Open namespace Jikes block
#endif

int counter1, counter2, counter3;
int nSingleton, nCoR, nBridge, nStrategy, nState, nFlyweight, nComposite, nMediator, nTemplate, nFactoryMethod, nAbstractFactory, nVisitor, nDecorator, nObserver, nProxy, nAdapter, nFacade;
bool PINOT_DEBUG;

SymbolSet mediators;
int nMediatorFacadeDual = 0, nFlyweightGoFVersion = 0, nImmutable = 0;
/**
 *	Utility functions	
 */

ContainerType *Utility::IdentifyContainerType(VariableSymbol *vsym)
{
	TypeSymbol *type = vsym->Type();

	if (type->Primitive())
		return NULL;

	if (type->IsArray())
		// can be 2D, 3D, etc.
		return new ArrayContainer(vsym);
	
	if (strcmp(type->fully_qualified_name->value, "java/util/Vector") == 0)
		return new VectorContainer(vsym);
	else if (strcmp(type->fully_qualified_name->value, "java/util/ArrayList") == 0)
		return new ArrayListContainer(vsym);
	else if (strcmp(type->fully_qualified_name->value, "java/util/LinkedList") == 0)
		return new ArrayListContainer(vsym);
	

       if (type->supertypes_closure)
       {
		Symbol *sym= type->supertypes_closure->FirstElement();
		while(sym)
		{
			if (strcmp(sym->TypeCast()->fully_qualified_name->value, "java/util/Map") == 0)
				return new MapContainer(vsym);
			else if (strcmp(sym->TypeCast()->fully_qualified_name->value, "java/util/Collection") == 0)
				return new CollectionContainer(vsym);
			sym = type->supertypes_closure->NextElement();
		}
       }
	return NULL;	
}
void Utility::RemoveJavaBaseClass(SymbolSet& set)
{
	Symbol *sym = set.FirstElement();
	while(sym)
	{
		if (strcmp(sym->TypeCast()->fully_qualified_name->value, "java/lang/Object") == 0)
		{
			set.RemoveElement(sym);
			break;
		}
		sym = set.NextElement();
	}
}
void Utility::RemoveBuiltinInterfaces(SymbolSet& set)
{
	SymbolSet temp;
	Symbol *sym = set.FirstElement();
	while(sym)
	{
		if (sym->TypeCast()->file_symbol->IsJava())
			temp.AddElement(sym);
		sym = set.NextElement();
	}
	set.Intersection(temp);
}
TypeSymbol *Utility::GetTypeSymbol(Symbol *sym)
{
	if (sym->Kind() == Symbol::TYPE)
		return sym->TypeCast();
	else if (sym->Kind() == Symbol::VARIABLE)
		return sym->VariableCast()->Type();
	else if (sym->Kind() == Symbol::METHOD)
		return sym->MethodCast()->Type();
	else
		return NULL;
}
AstExpression *Utility::RemoveCasting(AstExpression *expr)
{
	if (expr->kind == Ast::CAST)
		return RemoveCasting(expr->CastExpressionCast()->expression);
	else if (expr->kind == Ast::PARENTHESIZED_EXPRESSION)
		return RemoveCasting(expr->ParenthesizedExpressionCast()->expression);
	else
		return expr;
}
void Utility::Intersection(vector<signed>& a, vector<signed>& b, vector<signed>& c)
{
	for (unsigned i = 0; i < a.size(); i++)
		for (unsigned j = 0; j < b.size(); j++)
			if (a[i] == b[j])
				c.push_back(a[i]);
}
void Utility::RemoveDuplicates(vector<signed>& a)
{
	vector<signed> b;

	for (unsigned i = 0; i < a.size(); i++)
	{
		unsigned j = 0;
		while ((j < b.size()) && (a[i] != b[j])) j++;
		if (j == b.size())
			b.push_back(a[i]);
	}
	a.swap(b);
}
bool Utility::Aliasing(VariableSymbol *v1, VariableSymbol *v2)
{
	if (!v1->aliases)
		return false;
	else if (v1->aliases->IsElement(v2))
		return true;
	else
	{
		Symbol *sym = v1->aliases->FirstElement();
		while(sym)
		{
			// reach two-hops only
			if (sym->VariableCast()->aliases && sym->VariableCast()->aliases->IsElement(v2))
				return true;
			sym = v1->aliases->NextElement();
		}
		return false;
	}
}

bool isCached(wchar_t* name, vector<wchar_t*>* cache)
{
	bool flag = false;
	if (cache)
	{
		unsigned i = 0;
		while ((!flag) && (i < cache -> size()))
		{
			if (wcscmp((*cache)[i], name) == 0)
				flag = true;
			else
				i++;
		}
	}
	return flag;	
}

bool intersection(vector<wchar_t*>* list1, vector<wchar_t*>* list2)
{
	bool flag = false;
	if (list1 && list2)
	{
		unsigned i = 0, j;
		while (!flag && (i < list1 -> size()))
		{
			j = 0;
			while (!flag && (j < list2 -> size()))
			{
				if (wcscmp((*list1)[i], (*list2)[j]) == 0)
					flag = true;
				else
					j++;
			}
			if (!flag)
				i++;
		}	
	}
	return flag;
}

void printVector(vector<wchar_t*>* v)
{
	if (v)
	{
		unsigned i;
		for (i = 0; i < v -> size(); i++)
		{
			if (i > 0)
				Coutput << " ";
			Coutput << (*v)[i];
		}
		Coutput << endl;
	}
}

/**
 *	GoF patterns
 */
void PrintSingletonXMI(TypeSymbol *class_sym , VariableSymbol *instance_sym, MethodSymbol *method_sym);

bool DelegatesSuccessors(TypeSymbol *t1, TypeSymbol *t2)
{
	// pre-condition: t1 is concrete, while t2 is abstract

	if (t2->subtypes)
	{
		Symbol *sym = t2->subtypes->FirstElement();
		while (sym)
		{
			// checks for delegations to concrete classes.
			// the reason is to make sure that concrete strategies are not drectly exposed to the context class.
			if (!sym->TypeCast()->ACC_ABSTRACT() && sym->TypeCast()->call_dependents && sym->TypeCast()->call_dependents->IsElement(t1))
				return true;
			sym = t2->subtypes->NextElement();
		}
	}
	return false;
}

void FindThreadSafeInterface(DelegationTable *d_table)
{
	for (int i = 0; i < d_table -> size(); i++)
	{
		DelegationEntry *entry = d_table -> Entry(i);
		if (entry -> enclosing -> ACC_PRIVATE()
		&& (!entry -> base_opt || (entry -> base_opt -> kind == Ast::THIS_CALL))
		&& (entry -> enclosing == entry -> method))
		{
			Coutput << entry -> enclosing -> Utf8Name() << " is a private recursive function." << endl;
			Coutput << entry -> from -> file_symbol -> FileName() << endl;
		}
	}
}

/***
  *	XMI functions
  */

void PrintSingletonXMI(TypeSymbol *class_sym , VariableSymbol *instance_sym, MethodSymbol *method_sym)
{
    thread_local static ofstream fd("singleton.xmi");
    thread_local static int uid = 0;
    thread_local static int uuid = 32768;

	assert(fd.is_open());
	
	if (uid == 0)
	{
		uid++;
		fd << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
		fd << "<XMI xmi.version=\"1.0\">" << endl;
		fd << "  <XMI.header>" << endl;
		fd << "    <XMI.documentation>" << endl;
		fd << "      <XMI.exporter>Novosoft UML Library</XMI.exporter>" << endl;
		fd << "      <XMI.exporterVersion>0.4.20</XMI.exporterVersion>" << endl;
		fd << "    </XMI.documentation>" << endl;
		fd << "    <XMI.metamodel xmi.name=\"UML\" xmi.version=\"1.3\"/>" << endl;
		fd << "  </XMI.header>" << endl;
		fd << "  <XMI.content>" << endl;
		fd << "    <Model_Management.Model xmi.id=\"xmi." << uid++ << "\" xmi.uuid=\"-87--19-7--58-2b323e:102f1204eed:-8000\">" << endl;
		fd << "      <Foundation.Core.ModelElement.name>untitledModel</Foundation.Core.ModelElement.name>" << endl;
		fd << "      <Foundation.Core.Namespace.ownedElement>" << endl;
	}

	int cuid = uid, cuuid = uuid - 1;
	int iuid = cuid + 1, iuuid = cuuid - 1;
	int muid = iuid + 1, muuid = iuuid - 1;
	int ruid = muid + 1, ruuid = muuid - 1;

	uid += 4;
	uuid -= 4;

	if (1)
	{
		fd << "        <Foundation.Core.Class xmi.id=\"xmi." << cuid << "\" xmi.uuid=\"-87--19-7--58-2b323e:102f1204eed:-" << hex << cuuid << dec << "\">" << endl;
		fd << "          <Foundation.Core.ModelElement.name>" << class_sym -> Utf8Name() << "</Foundation.Core.ModelElement.name>" << endl;
		fd << "          <Foundation.Core.ModelElement.visibility xmi.value=\"public\"/>" << endl;
		fd << "          <Foundation.Core.GeneralizableElement.isAbstract xmi.value=\"" << ((class_sym -> ACC_ABSTRACT())?"true":"false") << "\"/>" << endl;
		fd << "          <Foundation.Core.Classifier.feature>" << endl;
		fd << "            <Foundation.Core.Attribute xmi.id=\"xmi." << iuid << "\" xmi.uuid=\"-87--19-7--58-2b323e:102f1204eed:-" << hex << iuuid << dec << "\">" << endl;
		fd << "              <Foundation.Core.ModelElement.name>" << instance_sym -> Utf8Name() << "</Foundation.Core.ModelElement.name>" << endl;
		fd << "              <Foundation.Core.ModelElement.visibility xmi.value=\"private\"/>" << endl;
		fd << "              <Foundation.Core.Feature.ownerScope xmi.value=\"classifier\"/>" << endl;
		fd << "              <Foundation.Core.Feature.owner>" << endl;
		fd << "                <Foundation.Core.Classifier xmi.idref=\"xmi." << cuid << "\"/>" << endl;
		fd << "              </Foundation.Core.Feature.owner>" << endl;
		fd << "              <Foundation.Core.StructuralFeature.type>" << endl;
		fd << "                <Foundation.Core.Classifier xmi.idref=\"xmi." << cuid << "\"/>" << endl;
		fd << "              </Foundation.Core.StructuralFeature.type>" << endl;
		fd << "            </Foundation.Core.Attribute>" << endl;
		fd << "            <Foundation.Core.Operation xmi.id=\"xmi." << muid << "\" xmi.uuid=\"-87--19-7--58-2b323e:102f1204eed:-" << hex << muuid << dec << "\">" << endl;
		fd << "              <Foundation.Core.ModelElement.name>" << method_sym -> Utf8Name() << "</Foundation.Core.ModelElement.name>" << endl;
		fd << "              <Foundation.Core.ModelElement.visibility xmi.value=\"public\"/>" << endl;
		fd << "              <Foundation.Core.Feature.ownerScope xmi.value=\"classifier\"/>" << endl;
		fd << "              <Foundation.Core.Operation.concurrency xmi.value=\"" << ((method_sym -> ACC_SYNCHRONIZED())?"concurrent":"sequential") << "\"/>" << endl;
		fd << "              <Foundation.Core.Feature.owner>" << endl;
		fd << "                <Foundation.Core.Classifier xmi.idref=\"xmi." << cuid << "\"/>" << endl;
		fd << "              </Foundation.Core.Feature.owner>" << endl;
		fd << "              <Foundation.Core.BehavioralFeature.parameter>" << endl;
		fd << "                <Foundation.Core.Parameter xmi.id=\"xmi." << ruid << "\" xmi.uuid=\"-87--19-7--58-2b323e:102f1204eed:-" << hex << ruuid << dec << "\">" << endl;
		fd << "                  <Foundation.Core.Parameter.kind xmi.value=\"return\"/>" << endl;
		fd << "                  <Foundation.Core.Parameter.type>" << endl;
		fd << "                    <Foundation.Core.Classifier xmi.idref=\"xmi." << cuid << "\"/>" << endl;
		fd << "                  </Foundation.Core.Parameter.type>" << endl;
		fd << "                </Foundation.Core.Parameter>" << endl;
		fd << "              </Foundation.Core.BehavioralFeature.parameter>" << endl;
		fd << "            </Foundation.Core.Operation>" << endl;
		fd << "          </Foundation.Core.Classifier.feature>" << endl;
		fd << "        </Foundation.Core.Class>" << endl;
	}
	if (uid == 14)
	{
		fd << "      </Foundation.Core.Namespace.ownedElement>" << endl;
		fd << "    </Model_Management.Model>" << endl;
		fd << "  </XMI.content>" << endl;
		fd << "</XMI>" << endl;
		fd.close();
	}
	
}



bool MapContainer::IsGetMethod(MethodSymbol *msym)
{
	return (strcmp(msym->Utf8Name(), "get") == 0);
}
bool MapContainer::IsPutMethod(MethodSymbol *msym)
{
	return ((strcmp(msym->Utf8Name(), "put") == 0) && (strcmp(msym->SignatureString(), "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;") == 0));
}
VariableSymbol *MapContainer::GetPutValue(AstMethodInvocation *call)
{
	return (*&call->arguments->Argument(1))->CastExpressionCast()->expression->symbol->VariableCast();
}
TypeSymbol *MapContainer::GetPutType(AstMethodInvocation *call)
{
	Symbol *sym = ((*&call->arguments->Argument(1))->kind == Ast::CAST)
		? (*&call->arguments->Argument(1))->CastExpressionCast()->expression->symbol
		: (*&call->arguments->Argument(1))->symbol;

	return Utility::GetTypeSymbol(sym);
}
bool CollectionContainer::IsPutMethod(MethodSymbol *msym)
{
	// check signature too? or No. Arguments
	return ((strcmp(msym->Utf8Name(), "add") == 0) && (strcmp(msym->SignatureString(), "(Ljava/lang/Object;)Z") == 0));
}
VariableSymbol *CollectionContainer::GetPutValue(AstMethodInvocation *call)
{
	return (*&call->arguments->Argument(0))->CastExpressionCast()->expression->symbol->VariableCast();
}
TypeSymbol *CollectionContainer::GetPutType(AstMethodInvocation *call)
{
	Symbol *sym = ((*&call->arguments->Argument(0))->kind == Ast::CAST)
		? (*&call->arguments->Argument(0))->CastExpressionCast()->expression->symbol
		: (*&call->arguments->Argument(0))->symbol;
	if (sym->Kind() == Symbol::TYPE)
		return sym->TypeCast();
	else if (sym->Kind() == Symbol::VARIABLE)
		return sym->VariableCast()->Type();
	else if (sym->Kind() == Symbol::METHOD)
		return sym->MethodCast()->Type();
	else
		return NULL;
}

bool ArrayListContainer::IsPutMethod(MethodSymbol *msym)
{
	if ((strcmp(msym->Utf8Name(), "add") == 0)
	&& ((strcmp(msym->SignatureString(), "(Ljava/lang/Object;)Z") == 0) || (strcmp(msym->SignatureString(), "(I;Ljava/lang/Object;)V") == 0)))
		return true;
	else
		return false;
}
bool LinkedListContainer::IsPutMethod(MethodSymbol *msym)
{
	if ((strcmp(msym->Utf8Name(), "add") == 0)
	&& ((strcmp(msym->SignatureString(), "(Ljava/lang/Object;)Z") == 0) || (strcmp(msym->SignatureString(), "(I;Ljava/lang/Object;)V") == 0)))
		return true;
	else if (((strcmp(msym->Utf8Name(), "addFirst") == 0) || (strcmp(msym->Utf8Name(), "addLast") == 0))
	&& ((strcmp(msym->SignatureString(), "(Ljava/lang/Object;)V") == 0)))
		return true;
	else
		return false;
}
bool VectorContainer::IsPutMethod(MethodSymbol *msym)
{
	if ((strcmp(msym->Utf8Name(), "add") == 0)
	&& ((strcmp(msym->SignatureString(), "(Ljava/lang/Object;)Z") == 0) || (strcmp(msym->SignatureString(), "(I;Ljava/lang/Object;)V") == 0)))
		return true;
	else if ((strcmp(msym->Utf8Name(), "addElement") == 0)
	&& ((strcmp(msym->SignatureString(), "(Ljava/lang/Object;)V") == 0)))
		return true;
	else
		return false;
}
bool HashSetContainer::IsPutMethod(MethodSymbol *msym)
{
	if ((strcmp(msym->Utf8Name(), "add") == 0) 
	&& (strcmp(msym->SignatureString(), "(Ljava/lang/Object;)Z") == 0))
		return true;
	else
		return false;
}

VariableSymbol *ArrayListContainer::GetPutValue(AstMethodInvocation *call)
{
	if (call->arguments->NumArguments() == 1)
		return (*&call->arguments->Argument(0))->CastExpressionCast()->expression->symbol->VariableCast();
	else
		return (*&call->arguments->Argument(1))->CastExpressionCast()->expression->symbol->VariableCast();
}
VariableSymbol *LinkedListContainer::GetPutValue(AstMethodInvocation *call)
{
	if (call->arguments->NumArguments() == 1)
		return (*&call->arguments->Argument(0))->CastExpressionCast()->expression->symbol->VariableCast();
	else
		return (*&call->arguments->Argument(1))->CastExpressionCast()->expression->symbol->VariableCast();
}
VariableSymbol *VectorContainer::GetPutValue(AstMethodInvocation *call)
{
	if (call->arguments->NumArguments() == 1)
		return (*&call->arguments->Argument(0))->CastExpressionCast()->expression->symbol->VariableCast();
	else
		return (*&call->arguments->Argument(1))->CastExpressionCast()->expression->symbol->VariableCast();
}
VariableSymbol *HashSetContainer::GetPutValue(AstMethodInvocation *call)
{
	return (*&call->arguments->Argument(0))->CastExpressionCast()->expression->symbol->VariableCast();
}

TypeSymbol *ArrayListContainer::GetPutType(AstMethodInvocation *call)
{
	AstExpression *expr = (call->arguments->NumArguments() == 1)
		? (*&call->arguments->Argument(0))
		: (*&call->arguments->Argument(1));

	Symbol *sym = (expr->kind == Ast::CAST)
		? expr->CastExpressionCast()->expression->symbol
		: expr->symbol;

	return Utility::GetTypeSymbol(sym);
}
TypeSymbol *LinkedListContainer::GetPutType(AstMethodInvocation *call)
{
	AstExpression *expr = (call->arguments->NumArguments() == 1)
		? (*&call->arguments->Argument(0))
		: (*&call->arguments->Argument(1));

	Symbol *sym = (expr->kind == Ast::CAST)
		? expr->CastExpressionCast()->expression->symbol
		: expr->symbol;

	return Utility::GetTypeSymbol(sym);
}
TypeSymbol *VectorContainer::GetPutType(AstMethodInvocation *call)
{
	AstExpression *expr = (call->arguments->NumArguments() == 1)
		? (*&call->arguments->Argument(0))
		: (*&call->arguments->Argument(1));

	Symbol *sym = (expr->kind == Ast::CAST)
		? expr->CastExpressionCast()->expression->symbol
		: expr->symbol;

	return Utility::GetTypeSymbol(sym);
}
TypeSymbol *HashSetContainer::GetPutType(AstMethodInvocation *call)
{
	AstExpression *expr = (*&call->arguments->Argument(0));

	Symbol *sym = (expr->kind == Ast::CAST)
		? expr->CastExpressionCast()->expression->symbol
		: expr->symbol;

	return Utility::GetTypeSymbol(sym);
}

void EmitExpressionAssociation(TypeSymbol * unit_type, MethodSymbol * enclosing_method, AstExpression * expression, DelegationTable * d_table, WriteAccessTable * w_table);
void EmitStatementAssociation(TypeSymbol * unit_type, MethodSymbol * enclosing_method, AstStatement * statement, DelegationTable * d_table, WriteAccessTable * w_table, ReadAccessTable *r_table);

void EmitGeneralization(GenTable * gen_table, TypeSymbol * unit_type)
{
	//AstClassBody* class_body = unit_type -> declaration;

    	wchar_t* package_name = unit_type -> FileLoc();
    	wchar_t* class_name = const_cast<wchar_t*>(unit_type -> Name());
	wchar_t* super_name = const_cast<wchar_t*>(unit_type -> super -> Name());
	
    	vector<wchar_t*>* interfaces = NULL;
	for (unsigned k = 0; k < unit_type -> NumInterfaces(); k++)
    	{
        	if (interfaces == NULL)
			interfaces = new vector<wchar_t*>();
  	 	interfaces -> push_back(const_cast<wchar_t*>(unit_type -> Interface(k) -> Name()));
    	}

	Gen::Kind kind;
	if (unit_type -> ACC_INTERFACE())
		kind = Gen::INTERFACE;
	else if (unit_type -> ACC_FINAL())
		kind = Gen::FINAL;
	else if (unit_type -> ACC_ABSTRACT())
		kind = Gen::ABSTRACT;
	else
		kind = Gen::CLASS;

	gen_table -> addGeneralization(package_name, class_name, super_name, interfaces, kind, const_cast<char*>(unit_type -> SignatureString()));
}

void EmitBlockAssociation(TypeSymbol * unit_type, MethodSymbol * enclosing_method, AstBlock * block, DelegationTable * d_table, WriteAccessTable * w_table, ReadAccessTable *r_table)
{	
	for (unsigned i = 0; i < block -> NumStatements(); i++)
		EmitStatementAssociation(unit_type, enclosing_method, block -> Statement(i), d_table, w_table, r_table);
}

void EmitDelegation(TypeSymbol * unit_type, MethodSymbol * enclosing_method, AstMethodInvocation * expression, DelegationTable * d_table, WriteAccessTable * w_table)
{
	//
	// If the method call was resolved into a call to another method, use the
    	// resolution expression.
    	//
    	AstMethodInvocation* method_call = expression -> resolution_opt
    		? expression -> resolution_opt -> MethodInvocationCast() : expression;	
	assert(method_call);

	MethodSymbol *msym = method_call->symbol->MethodCast();
	if(!msym) return; // skip bad tokens

	VariableSymbol *vsym = (method_call -> base_opt 
							&& method_call -> base_opt -> kind == Ast::NAME 
							&& (method_call -> base_opt -> symbol -> Kind() == Symbol::VARIABLE)) 
						? method_call -> base_opt -> symbol -> VariableCast()
						: NULL;


	d_table -> InsertDelegation(unit_type, msym -> containing_type, method_call -> base_opt, vsym, msym, enclosing_method, method_call);
	if (!msym -> containing_type -> call_dependents)
		msym -> containing_type -> call_dependents = new SymbolSet(0);
	msym -> containing_type -> call_dependents -> AddElement(unit_type);
	if (!msym -> callers)
		msym -> callers = new SymbolSet(0);
	msym -> callers -> AddElement(unit_type);
	
	if (!msym -> invokers)
		msym -> invokers = new SymbolSet(0);
	msym -> invokers -> AddElement(enclosing_method);
	
	if (!enclosing_method -> invokees)
		enclosing_method -> invokees = new SymbolSet(0);
	enclosing_method -> invokees -> AddElement(msym);

	if (!unit_type -> associates)
		unit_type -> associates = new SymbolSet(0);
	unit_type -> associates -> AddElement(msym -> containing_type);
	
	if (method_call -> base_opt)
		EmitExpressionAssociation(unit_type, enclosing_method, method_call -> base_opt, d_table, w_table);
	
	AstArguments *args = expression -> arguments;
	for (unsigned i = 0; i < args -> NumArguments(); i++)
	{
		if (args->Argument(i)->symbol->VariableCast())
		{
			if (!msym->FormalParameter(i)->aliases)
				msym->FormalParameter(i)->aliases = new SymbolSet();
			msym->FormalParameter(i)->aliases->AddElement(args->Argument(i)->symbol->VariableCast());
		}
		EmitExpressionAssociation(unit_type, enclosing_method, args -> Argument(i), d_table, w_table);
	}
}

void EmitReadAccess(TypeSymbol * unit_type, MethodSymbol * enclosing_method, AstName * name, ReadAccessTable * r_table)
{
	if (name -> symbol -> Kind() == Symbol::VARIABLE)
	{
		VariableSymbol *vsym = name -> symbol -> VariableCast();
		if (vsym -> IsLocal())
			vsym = unit_type -> Shadows(vsym);
		if (vsym)
			r_table -> InsertReadAccess(vsym, enclosing_method);
	}
}

void EmitWriteAccess(TypeSymbol * unit_type, MethodSymbol * enclosing_method, AstAssignmentExpression * assignment, DelegationTable * d_table, WriteAccessTable * w_table)
{
	AstExpression *left_expression = assignment->left_hand_side;
	VariableSymbol *vsym = NULL;
	
	if (left_expression->kind == Ast::DOT)
		vsym = left_expression->FieldAccessCast()->symbol->VariableCast();
	else if ((left_expression -> kind == Ast::NAME) && (left_expression->symbol->Kind()==Symbol::VARIABLE))
	{
	/*
		left_expression = (left_expression->NameCast()->resolution_opt)
			? left_expression->NameCast()->resolution_opt
			: left_expression;
	*/
		vsym = left_expression -> symbol -> VariableCast();
	}
#ifdef GOF_CONSOLE
		if (vsym -> ContainingType() == unit_type)
			Coutput << vsym -> ContainingType() -> Utf8Name() 
				<< "::" 
				<< enclosing_method -> Utf8Name() 
				<< " accesses a private field " 
				<< vsym -> Utf8Name() 
				<< ": "
				<< vsym -> Type () -> Utf8Name()
				<< endl;
#endif
	if (vsym)
	{
		w_table -> InsertWriteAccess(vsym, enclosing_method);

		/*
		AstExpression *rhs_expression = (assignment-> expression -> kind == Ast::CAST) 
			? assignment-> expression -> CastExpressionCast() -> expression
			: assignment-> expression;
		*/

		AstExpression *rhs_expression = Utility::RemoveCasting(assignment-> expression);

		if (vsym && rhs_expression -> kind == Ast::CLASS_CREATION)
		{
				if (!vsym -> concrete_types)
					vsym -> concrete_types = new SymbolSet(0);
				vsym -> concrete_types -> AddElement(rhs_expression -> ClassCreationExpressionCast() -> class_type -> symbol -> TypeCast());
		}
		if (rhs_expression->symbol->VariableCast())
		{
			if (!vsym->aliases)
				vsym->aliases = new SymbolSet();
			vsym->aliases->AddElement(rhs_expression->symbol->VariableCast());
		}
		
	}
	EmitExpressionAssociation(unit_type, enclosing_method, assignment-> expression, d_table, w_table);
}

void EmitExpressionAssociation(TypeSymbol * unit_type, MethodSymbol * enclosing_method, AstExpression * expression, DelegationTable * d_table, WriteAccessTable * w_table)
{
	switch(expression -> kind)
	{
		case Ast::CLASS_CREATION:
			if (!unit_type -> associates)
				unit_type -> associates = new SymbolSet(0);
			if (expression->ClassCreationExpressionCast()->class_type->symbol)
				unit_type -> associates -> AddElement(((AstClassCreationExpression*)expression) -> class_type -> symbol -> TypeCast());
			else
				unit_type->associates->AddElement(expression->ClassCreationExpressionCast()->symbol->MethodCast()->containing_type);
			break;
		case Ast::CALL:
		    if(!expression->MethodInvocationCast()) break;
			EmitDelegation(unit_type, enclosing_method, expression->MethodInvocationCast(), d_table, w_table);
			break;
		case Ast::ASSIGNMENT:
		    if(!expression->AssignmentExpressionCast()) break;
			EmitWriteAccess(unit_type, enclosing_method, expression->AssignmentExpressionCast(), d_table, w_table);
			EmitExpressionAssociation(unit_type, enclosing_method, expression->AssignmentExpressionCast() -> expression, d_table, w_table);
			break;
		case Ast::CONDITIONAL:
		    if(!expression->ConditionalExpressionCast()) break;
			EmitExpressionAssociation(unit_type, enclosing_method, expression->ConditionalExpressionCast()-> test_expression, d_table, w_table);
			EmitExpressionAssociation(unit_type, enclosing_method, expression->ConditionalExpressionCast() -> true_expression, d_table, w_table);
			EmitExpressionAssociation(unit_type, enclosing_method, expression->ConditionalExpressionCast()-> false_expression, d_table, w_table);
			break;
		case Ast::CAST:
		    if(!expression->CastExpressionCast()) break;
			EmitExpressionAssociation(unit_type, enclosing_method, expression->CastExpressionCast()-> expression, d_table, w_table);
			break;
		case Ast::PARENTHESIZED_EXPRESSION:
		    if(!expression->ParenthesizedExpressionCast()) break;
			EmitExpressionAssociation(unit_type, enclosing_method, expression->ParenthesizedExpressionCast()-> expression, d_table, w_table);
			break;
		case Ast::BINARY:
		    if(!expression->BinaryExpressionCast()) break;
			EmitExpressionAssociation(unit_type, enclosing_method, expression->BinaryExpressionCast()-> left_expression, d_table, w_table);
			EmitExpressionAssociation(unit_type, enclosing_method, expression->BinaryExpressionCast()-> right_expression, d_table, w_table);
			break;			
		default:
			break;
	}
}

void EmitStatementAssociation(TypeSymbol *unit_type, MethodSymbol *enclosing_method, AstStatement *statement,
                              DelegationTable *d_table, WriteAccessTable *w_table, ReadAccessTable *r_table) {
    switch (statement->kind) {
        case Ast::METHOD_BODY:
        case Ast::BLOCK: // JLS 14.2
        {
            EmitBlockAssociation(unit_type, enclosing_method, (AstBlock *) statement, d_table, w_table, r_table);
        }
            break;
        case Ast::LOCAL_VARIABLE_DECLARATION: // JLS 14.3
        {
            auto local = statement->LocalVariableStatementCast();
            for (unsigned i = 0; i < local->NumVariableDeclarators(); i++)
                EmitStatementAssociation(unit_type, enclosing_method, local->VariableDeclarator(i), d_table, w_table,
                                         r_table);
        }
            break;
        case Ast::EMPTY_STATEMENT: // JLS 14.5
            break;
        case Ast::EXPRESSION_STATEMENT: // JLS 14.7
        {
            EmitExpressionAssociation(unit_type, enclosing_method, statement->ExpressionStatementCast()->expression,
                                      d_table, w_table);
        }
            break;
        case Ast::IF: // JLS 14.8
        {
            AstIfStatement *if_statement = statement->IfStatementCast();
            EmitExpressionAssociation(unit_type, enclosing_method, if_statement->expression, d_table, w_table);
            EmitBlockAssociation(unit_type, enclosing_method, if_statement->true_statement, d_table, w_table, r_table);
            if (if_statement->false_statement_opt)
                EmitBlockAssociation(unit_type, enclosing_method, if_statement->false_statement_opt, d_table, w_table,
                                     r_table);
        }
            break;
        case Ast::SWITCH: // JLS 14.9
        {
            AstSwitchStatement *cp = statement->SwitchStatementCast();
            EmitExpressionAssociation(unit_type, enclosing_method, cp->expression, d_table, w_table);
            EmitBlockAssociation(unit_type, enclosing_method, cp->switch_block, d_table, w_table, r_table);
        }
            break;
        case Ast::SWITCH_BLOCK: // JLS 14.9
        {
            EmitBlockAssociation(unit_type, enclosing_method, statement->BlockCast(), d_table, w_table, r_table);
        }
            break;
        case Ast::SWITCH_LABEL:
            break;
        case Ast::WHILE: // JLS 14.10
        {
            AstWhileStatement *wp = statement->WhileStatementCast();
            EmitExpressionAssociation(unit_type, enclosing_method, wp->expression, d_table, w_table);
            EmitBlockAssociation(unit_type, enclosing_method, wp->statement, d_table, w_table, r_table);
        }
            break;
        case Ast::DO: // JLS 14.11
        {
            AstDoStatement *sp = statement->DoStatementCast();
            EmitExpressionAssociation(unit_type, enclosing_method, sp->expression, d_table, w_table);
            EmitBlockAssociation(unit_type, enclosing_method, sp->statement, d_table, w_table, r_table);
        }
            break;
        case Ast::FOR: // JLS 14.12
        {
            AstForStatement *for_statement = statement->ForStatementCast();
            if (for_statement->end_expression_opt)
                EmitExpressionAssociation(unit_type, enclosing_method, for_statement->end_expression_opt, d_table,
                                          w_table);
            unsigned i;
            for (i = 0; i < for_statement->NumForInitStatements(); i++)
                EmitStatementAssociation(unit_type, enclosing_method, for_statement->ForInitStatement(i), d_table,
                                         w_table, r_table);
            for (i = 0; i < for_statement->NumForUpdateStatements(); i++)
                EmitStatementAssociation(unit_type, enclosing_method, for_statement->ForUpdateStatement(i), d_table,
                                         w_table, r_table);
            EmitBlockAssociation(unit_type, enclosing_method, for_statement->statement, d_table, w_table, r_table);
        }
            break;
        case Ast::FOREACH: // JSR 201
        case Ast::BREAK: // JLS 14.13
        case Ast::CONTINUE: // JLS 14.14
            break;
        case Ast::RETURN: // JLS 14.15
        {
            AstReturnStatement *rp = statement->ReturnStatementCast();
            if (rp->expression_opt) {
                if (rp->expression_opt->kind == Ast::NAME)
                    EmitReadAccess(unit_type, enclosing_method, rp->expression_opt->NameCast(), r_table);
                else
                    EmitExpressionAssociation(unit_type, enclosing_method, rp->expression_opt, d_table, w_table);
            }
        }
            break;
        case Ast::SUPER_CALL:
        case Ast::THIS_CALL:
        case Ast::THROW: // JLS 14.16
            break;
        case Ast::SYNCHRONIZED_STATEMENT: // JLS 14.17
        {
            EmitBlockAssociation(unit_type, enclosing_method, statement->SynchronizedStatementCast()->block, d_table,
                                 w_table, r_table);
        }
            break;
        case Ast::TRY: // JLS 14.18
        {
            EmitBlockAssociation(unit_type, enclosing_method, statement->TryStatementCast()->block, d_table, w_table,
                                 r_table);
        }
            break;
        case Ast::CATCH:   // JLS 14.18
        case Ast::FINALLY: // JLS 14.18
        case Ast::ASSERT: // JDK 1.4 (JSR 41)
        case Ast::LOCAL_CLASS: // Class Declaration
            //
            // This is factored out by the front end; and so must be
            // skipped here (remember, interfaces cannot be declared locally).
            //
            break;
        case Ast::VARIABLE_DECLARATOR: {
            AstVariableDeclarator *vd = statement->VariableDeclaratorCast();
            if (vd->variable_initializer_opt && vd->variable_initializer_opt->ExpressionCast()) {
                AstExpression *rhs_expression = Utility::RemoveCasting(vd->variable_initializer_opt->ExpressionCast());
                if (rhs_expression->symbol->VariableCast()) {
                    if (!vd->symbol->aliases)
                        vd->symbol->aliases = new SymbolSet();
                    vd->symbol->aliases->AddElement(rhs_expression->symbol->VariableCast());
                }

                EmitExpressionAssociation(unit_type, enclosing_method, vd->variable_initializer_opt->ExpressionCast(),
                                          d_table, w_table);
            }
        }
            break;
        default:
            break;
    }
}

void ExtractStructure(WriteAccessTable *w_table, ReadAccessTable *r_table, DelegationTable *d_table,
                      ClassSymbolTable *cs_table, MethodBodyTable *mb_table, MethodSymbolTable *ms_table,
                      GenTable *gen_table, AssocTable *assoc_table, TypeSymbol *unit_type, StoragePool *ast_pool) {
    //Coutput << unit_type->fully_qualified_name->value << endl;
    if (unit_type->Anonymous() && (unit_type->NumInterfaces() || unit_type->super)) {
        if (!unit_type->supertypes_closure)
            unit_type->supertypes_closure = new SymbolSet(0);
        if (unit_type->NumInterfaces())
            unit_type->supertypes_closure->AddElement(unit_type->Interface(0));
        if (unit_type->super)
            unit_type->supertypes_closure->AddElement(unit_type->super);
    }

    Semantic &semantic = *unit_type->semantic_environment->sem;
    LexStream *lex_stream = semantic.lex_stream;

    wchar_t *package_name = unit_type->FileLoc();

    AstClassBody *class_body = unit_type->declaration;

    class_body->Lexify(*lex_stream);

    wchar_t *class_name = const_cast<wchar_t *>(unit_type->Name());

    EmitGeneralization(gen_table, unit_type); // to be eliminated.
    cs_table->AddClassSymbol(unit_type);

    unsigned i;

    if ((class_body->NumClassVariables() + class_body->NumInstanceVariables()) > 0) {
        unit_type->instances = new SymbolSet();
        unit_type->references = new SymbolSet();
    }

    //
    // Process static variables.
    //
    for (i = 0; i < class_body->NumClassVariables(); i++) {
        AstFieldDeclaration *field_decl = class_body->ClassVariable(i);

        TypeSymbol *type = (field_decl->type->symbol->IsArray())
                           ? field_decl->type->symbol->base_type
                           : field_decl->type->symbol;
        unit_type->references->AddElement(type);

        for (unsigned vi = 0;
             vi < field_decl->NumVariableDeclarators(); vi++) {
            AstVariableDeclarator *vd = field_decl->VariableDeclarator(vi);
            unit_type->instances->AddElement(vd->symbol);
            field_decl->PrintAssociation(assoc_table, package_name, class_name, *lex_stream);
            //DeclareField(vd -> symbol);

            if (vd->variable_initializer_opt && vd->variable_initializer_opt->ExpressionCast()) {
                AstExpression *rhs_expression = Utility::RemoveCasting(vd->variable_initializer_opt->ExpressionCast());
                if (rhs_expression->symbol->VariableCast()) {
                    if (!vd->symbol->aliases)
                        vd->symbol->aliases = new SymbolSet();
                    vd->symbol->aliases->AddElement(rhs_expression->symbol->VariableCast());
                }
            }
        }
    }

    //
    // Process instance variables.  We separate constant fields from others,
    // because in 1.4 or later, constant fields are initialized before the
    // call to super() in order to obey semantics of JLS 13.1.
    //
    Tuple<AstVariableDeclarator *> constant_instance_fields
            (unit_type->NumVariableSymbols());
    for (i = 0; i < class_body->NumInstanceVariables(); i++) {
        AstFieldDeclaration *field_decl = class_body->InstanceVariable(i);

        TypeSymbol *type = (field_decl->type->symbol->IsArray())
                           ? field_decl->type->symbol->base_type
                           : field_decl->type->symbol;
        unit_type->references->AddElement(type);

        for (unsigned vi = 0;
             vi < field_decl->NumVariableDeclarators(); vi++) {
            AstVariableDeclarator *vd = field_decl->VariableDeclarator(vi);
            field_decl->PrintAssociation(assoc_table, package_name, class_name, *lex_stream);
            VariableSymbol *vsym = vd->symbol;
            unit_type->instances->AddElement(vsym);
            //DeclareField(vsym);
            if (vd->variable_initializer_opt && vsym->initial_value) {
                AstExpression *init;
                assert(init = vd->variable_initializer_opt->ExpressionCast());
                assert(init->IsConstant() && vd->symbol->ACC_FINAL());
                constant_instance_fields.Next() = vd;

                AstExpression *expr = (init->kind == Ast::CAST)
                                      ? init->CastExpressionCast()->expression
                                      : init;
                if (expr->kind == Ast::CLASS_CREATION) {
                    if (!vsym->concrete_types)
                        vsym->concrete_types = new SymbolSet(0);
                    vsym->concrete_types->AddElement(
                            expr->ClassCreationExpressionCast()->class_type->symbol->TypeCast());
                }
            }
            if (vd->variable_initializer_opt && vd->variable_initializer_opt->ExpressionCast()) {
                AstExpression *rhs_expression = Utility::RemoveCasting(vd->variable_initializer_opt->ExpressionCast());
                if (rhs_expression->symbol->VariableCast()) {
                    if (!vd->symbol->aliases)
                        vd->symbol->aliases = new SymbolSet();
                    vd->symbol->aliases->AddElement(rhs_expression->symbol->VariableCast());
                }
            }
        }
    }

    //
    // Process synthetic fields (this$0, local shadow parameters, $class...,
    // $array..., $noassert).
    //
    /*
    if (unit_type -> EnclosingType())
        DeclareField(unit_type -> EnclosingInstance());
    for (i = 0; i < unit_type -> NumConstructorParameters(); i++)
        DeclareField(unit_type -> ConstructorParameter(i));
    for (i = 0; i < unit_type -> NumClassLiterals(); i++)
        DeclareField(unit_type -> ClassLiteral(i));
    VariableSymbol* assert_variable = unit_type -> AssertVariable();
    if (assert_variable)
    {
        assert(! control.option.noassert);
        DeclareField(assert_variable);
        if (control.option.target < JikesOption::SDK1_4)
        {
            semantic.ReportSemError(SemanticError::ASSERT_UNSUPPORTED_IN_TARGET,
                                    unit_type -> declaration,
                                    unit_type -> ContainingPackageName(),
                                    unit_type -> ExternalName());
            assert_variable = NULL;
        }
    }
    */
    //
    // Process declared methods.
    //
    for (i = 0; i < class_body->NumMethods(); i++) {
        AstMethodDeclaration *method = class_body->Method(i);
        if (method->method_symbol) {
            counter3++;
            wchar_t *method_name = const_cast<wchar_t *>((*lex_stream).NameString(
                    method->method_declarator->identifier_token));

            //int method_index = methods.NextIndex(); // index for method
            //BeginMethod(method_index, method -> method_symbol);
            if (method->method_body_opt) // not an abstract method ?
            {
                mb_table->addMethodBodyAddr(package_name, class_name, method_name, method);  // to be eliminated.
                method->PrintAssociation(assoc_table, package_name, class_name, *lex_stream);
                ms_table->AddMethodSymbol(method->method_symbol);

                assert(method->method_body_opt->NumStatements() > 0);
                EmitBlockAssociation(unit_type, method->method_symbol, method->method_body_opt, d_table, w_table,
                                     r_table);

                counter2++;
                //EmitBlockStatement(method -> method_body_opt);
            } else
                counter1++;
            //EndMethod(method_index, method -> method_symbol);
        }
    }

    //
    // Process synthetic methods (access$..., class$).
    //
    /*
    for (i = 0; i < unit_type -> NumPrivateAccessMethods(); i++)
    {
        int method_index = methods.NextIndex(); // index for method
        MethodSymbol* method_sym = unit_type -> PrivateAccessMethod(i);
        AstMethodDeclaration* method = method_sym -> declaration ->
            MethodDeclarationCast();
        assert(method);
        BeginMethod(method_index, method_sym);
        EmitBlockStatement(method -> method_body_opt);
        EndMethod(method_index, method_sym);
    }
    MethodSymbol* class_literal_sym = unit_type -> ClassLiteralMethod();
    if (class_literal_sym)
    {
        int method_index = methods.NextIndex(); // index for method
        BeginMethod(method_index, class_literal_sym);
        GenerateClassAccessMethod();
        EndMethod(method_index, class_literal_sym);
    }
    */
    //
    // Process the instance initializer.
    //
    /*
    bool has_instance_initializer = false;
    if (unit_type -> instance_initializer_method)
    {
        AstMethodDeclaration* declaration = (AstMethodDeclaration*)
            unit_type -> instance_initializer_method -> declaration;
        AstBlock* init_block = declaration -> method_body_opt;
        if (! IsNop(init_block))
        {
            int method_index = methods.NextIndex(); // index for method
            BeginMethod(method_index,
                        unit_type -> instance_initializer_method);
            bool abrupt = EmitBlockStatement(init_block);
            if (! abrupt)
                PutOp(OP_RETURN);
            EndMethod(method_index, unit_type -> instance_initializer_method);
            has_instance_initializer = true;
        }
    }
    */
    //
    // Process all constructors (including synthetic ones).
    //

    if (!class_body->default_constructor) {
        for (i = 0; i < class_body->NumConstructors(); i++) {
            //AstConstructorDeclaration *constructor = dynamic_cast<AstConstructorDeclaration*>(class_body -> Constructor(i) -> Clone(ast_pool, *lex_stream));
            AstConstructorDeclaration *constructor = class_body->Constructor(i);
            mb_table->addMethodBodyAddr(package_name, class_name, class_name, constructor);  // to be eliminated.
            ms_table->AddMethodSymbol(constructor->constructor_symbol);
            EmitBlockAssociation(unit_type, constructor->constructor_symbol, constructor->constructor_body, d_table,
                                 w_table, r_table);

            // CompileConstructor(class_body -> Constructor(i), constant_instance_fields, has_instance_initializer);
        }
    }
    /*
    for (i = 0; i < unit_type -> NumPrivateAccessConstructors(); i++)
    {
    
		Coutput << "private access class ctor: " << class_name << endl; 
	
          MethodSymbol* constructor_sym = unit_type -> PrivateAccessConstructor(i);
          AstConstructorDeclaration* constructor = constructor_sym -> declaration -> ConstructorDeclarationCast();

 	   mb_table -> addMethodBodyAddr(package_name, class_name, class_name, constructor);
	   constructor-> PrintAssociation(assoc_table, package_name, class_name, *lex_stream);
	   ms_table -> AddMethodSymbol(const_cast<char*>(constructor_sym -> Utf8Name()), constructor_sym);

         // CompileConstructor(constructor, constant_instance_fields, has_instance_initializer);
    }
    */
    //
    // Process the static initializer.
    //
    /*
    if (unit_type -> static_initializer_method)
    {
        AstMethodDeclaration* declaration = (AstMethodDeclaration*)
            unit_type -> static_initializer_method -> declaration;
        AstBlock* init_block = declaration -> method_body_opt;
        if (assert_variable || ! IsNop(init_block))
        {
            int method_index = methods.NextIndex(); // index for method
            BeginMethod(method_index, unit_type -> static_initializer_method);
            if (assert_variable)
                GenerateAssertVariableInitializer(unit_type -> outermost_type,
                                                  assert_variable);
            bool abrupt = EmitBlockStatement(init_block);
            if (! abrupt)
                PutOp(OP_RETURN);
            EndMethod(method_index, unit_type -> static_initializer_method);
        }
    }
    */
}

#ifdef DONTDOIT
void PrintRelation(MethodBodyTable* mb_table, GenTable* gen_table, AssocTable* assoc_table, TypeSymbol* unit_type, StoragePool* ast_pool)
{
	Semantic& semantic = *unit_type -> semantic_environment -> sem;
    	LexStream *lex_stream = semantic.lex_stream;

    	AstCompilationUnit* compilation_unit = semantic.compilation_unit;

     	unsigned i;
     	for (i = 0; i < compilation_unit -> NumTypeDeclarations(); i++)
     	{
		wchar_t* package_name = (compilation_unit -> package_declaration_opt)
			? const_cast<wchar_t*>((*lex_stream).NameString(compilation_unit -> package_declaration_opt -> name -> identifier_token))
			: NULL;

		if (compilation_unit -> TypeDeclaration(i) -> kind == Ast::CLASS)
         	{
             		AstClassDeclaration* class_declaration = dynamic_cast<AstClassDeclaration*> (compilation_unit -> TypeDeclaration(i));
             		class_declaration -> PrintGeneralization(gen_table, package_name, *lex_stream);

             		AstClassBody* class_body = class_declaration -> class_body;
             		unsigned j;
             		for (j = 0; j < class_body -> NumClassBodyDeclarations(); j++)
             		{
                 		switch(class_body -> ClassBodyDeclaration(j) -> kind)
                 		{
                     		case Ast::FIELD:
                             		class_body -> ClassBodyDeclaration(j) -> PrintAssociation(assoc_table, 
																		  package_name,
																		  const_cast<wchar_t*>((*lex_stream).NameString(class_body -> identifier_token)), 
																		  *lex_stream);
						break;
                     		case Ast::CONSTRUCTOR:
						{
						wchar_t* class_name = const_cast<wchar_t*>((*lex_stream).NameString(class_body -> identifier_token));

  	                                   AstConstructorDeclaration* ctor_declaration = dynamic_cast<AstConstructorDeclaration*> (class_body -> ClassBodyDeclaration(j));
						wchar_t* ctor_name = const_cast<wchar_t*>((*lex_stream).NameString(ctor_declaration -> constructor_declarator -> identifier_token));

						if (!ctor_declaration -> GoFTag)
						{
							AstConstructorDeclaration* cloned_declaration = dynamic_cast<AstConstructorDeclaration*>(ctor_declaration -> Clone(ast_pool, *lex_stream));
							mb_table -> addMethodBodyAddr(package_name, class_name, ctor_name, cloned_declaration);
							ctor_declaration -> GoFTag = true;
                     			}
                     			}
						break;
                     		case Ast::METHOD:
                         			{
						wchar_t* _className = const_cast<wchar_t*>((*lex_stream).NameString(class_body -> identifier_token));

  	                                   AstMethodDeclaration* method_declaration = DYNAMIC_CAST<AstMethodDeclaration*> (class_body -> ClassBodyDeclaration(j));
						wchar_t* _methodName = const_cast<wchar_t*>((*lex_stream).NameString(method_declaration -> method_declarator -> identifier_token));

/*
						if ((wcscmp(_className, L"MediaTracker") == 0)
						&& (wcscmp(_methodName, L"addImage") == 0))
							method_declaration -> method_body_opt -> Statement(0) -> Print(*lex_stream);
*/
						if (!method_declaration -> method_body_opt)
							counter1++;
						else if (method_declaration -> method_body_opt -> NumStatements() > 0)
							counter2++;

						else if ((method_declaration -> method_body_opt)
						&& (method_declaration -> method_body_opt -> NumStatements() == 0))
							Coutput << _className << "." << _methodName << endl;

						counter3++;

						AstMethodDeclaration* cloned_declaration = DYNAMIC_CAST<AstMethodDeclaration*>(method_declaration -> Clone(ast_pool, *lex_stream));
						mb_table -> addMethodBodyAddr(package_name, _className, _methodName, cloned_declaration);			
						
                              		class_body -> ClassBodyDeclaration(j) -> PrintAssociation(assoc_table, 
																		  package_name,
																		  const_cast<wchar_t*>((*lex_stream).NameString(class_body -> identifier_token)), 
																		  *lex_stream);
						method_declaration -> GoFTag = true;
                         			}
                         			break;
					case Ast::CLASS:
					case Ast::INTERFACE:
						{
							class_body -> ClassBodyDeclaration(j) -> PrintGeneralization(gen_table, package_name, *lex_stream);
						}
						break;
                     		default:
                         			break;
                 		} // switch
             		} // for
             		class_declaration -> GoFTag = true;
         	}
		else if (compilation_unit -> TypeDeclaration(i) -> kind == Ast::INTERFACE)
		{
			AstInterfaceDeclaration* interface_declaration = dynamic_cast<AstInterfaceDeclaration*> (compilation_unit -> TypeDeclaration(i));
			interface_declaration -> PrintGeneralization(gen_table, package_name, *lex_stream);
			interface_declaration -> GoFTag = true;
		}
		else
		{
			//Coutput << L"kind = " << compilation_unit -> TypeDeclaration(i) -> kind << endl;
		}
     	}
}
#endif

bool WriteAccessTable::IsWrittenBy(VariableSymbol *vsym, MethodSymbol *msym)
{
	multimap<VariableSymbol*, MethodSymbol*>::iterator p;
	for (p = table -> begin(); p != table -> end(); p++)
	{
		if ((p -> first == vsym) && (p -> second == msym))
			return true;
	}
	return false;
}

bool WriteAccessTable::IsWrittenBy(VariableSymbol *vsym, MethodSymbol *msym, DelegationTable *d_table)
{
	multimap<VariableSymbol*, MethodSymbol*>::iterator p;
	for (p = table -> begin(); p != table -> end(); p++)
	{
		if (p -> first == vsym)
		{ 
			if ((p -> second == msym) || (d_table -> TraceCall(p -> second, msym)))
				return true;				
		}		
	}
	return false;
}

int DelegationTable::IsBidirectional(TypeSymbol *t1,TypeSymbol *t2)
{
	//
	// return code: 
	//
	// 		3: bidirectional
	// 		2: -->
	// 		1: <--
	// 		0: no delegation
	//

	int forward = 0, backward = 0;
	for (unsigned i =0; i < table -> size(); i++)
	{
		DelegationEntry *entry = (*table)[i];
		forward =  ((forward == 0) && (entry->from == t1) && (entry -> to == t2)) ? 1 : forward;
		backward =  ((backward == 0) && (entry->from == t2) && (entry -> to == t1)) ?  1 : backward;
	}
	return (2*forward + backward);	
}

bool DelegationTable::TraceCall(MethodSymbol *start, MethodSymbol *target)
{
	for (unsigned i =0; i < table -> size(); i++)
	{
		DelegationEntry *entry = (*table)[i];
		if (entry -> method == start)
		{
			if ((entry -> enclosing == target) 
			||(entry -> enclosing -> Overrides(target))
			|| TraceCall(entry -> enclosing, target))
				return true;
		}
	}
	return false;
}

int DelegationTable::UniqueDirectedCalls ()
{
	multimap<TypeSymbol*, TypeSymbol*> stack;

	for (unsigned i = 0; i < table -> size(); i++)
	{
		DelegationEntry *entry = (*table)[i];

		if (entry -> to -> file_symbol
		&& (!entry -> to -> file_symbol -> IsClassOnly())
		&& (entry -> from != entry -> to)
		&& ((!entry -> from -> IsSubclass(entry -> to)) && (!entry -> to -> IsSubclass(entry -> from))))
		{
			if (stack.size() > 0)
			{
				multimap<TypeSymbol*, TypeSymbol*>::iterator p = stack.begin();
				while (((p->first != entry -> from) || (p->second != entry -> to))					
					&& (p != stack.end()))
					p++;
				if (p == stack.end())
					stack.insert(p, pair<TypeSymbol*, TypeSymbol*>(entry->from, entry->to));
			}
			else
				stack.insert(pair<TypeSymbol*, TypeSymbol*>(entry->from, entry->to));
		}
	}

	return stack.size();
}

bool DelegationTable::DelegatesSuccessors(TypeSymbol *from, TypeSymbol *to)
{
	assert (!from -> ACC_INTERFACE() && to -> ACC_INTERFACE());

	for (unsigned i = 0; i < table -> size(); i++)
	{
		DelegationEntry *entry = (*table)[i];
		if ((entry -> from == from)  && (entry -> to != to) && (entry -> to -> Implements(to)))
		{
			TypeSymbol *resolve = (entry -> base_opt) ? ResolveType(entry -> base_opt) : NULL;
			if (resolve && (to != resolve))
			{
#ifdef GOF_CONSOLE			
					Coutput << "From: " << from -> Utf8Name() 
						<< " To: " << to -> Utf8Name() 
						<< " RESOLVE: " << resolve -> Utf8Name() 
						<< " (" << entry -> method -> Utf8Name() << ")" 
						<< endl;
#endif			
					return true;
			}
		}
	}
	return false;
}

TypeSymbol *DelegationTable::ResolveType(AstExpression *expression)
{
	assert (expression);
	
	switch(expression -> kind)
	{
		case Ast::NAME:
			if (expression -> NameCast() -> resolution_opt)
				return ResolveType(expression -> NameCast() -> resolution_opt);
			else if (expression -> symbol -> Kind() == Symbol::TYPE)
				return expression -> symbol -> TypeCast();
			else if (expression -> symbol -> Kind() == Symbol::VARIABLE)
			{
				VariableSymbol *vsym = expression -> symbol -> VariableCast();
				VariableSymbol *svsym = vsym -> ContainingType() -> Shadows(vsym);
				if (svsym)
					return svsym -> Type();
				else
				{
//					Coutput << "\"" << vsym -> FileLoc() << "\"" << endl;
					if (vsym -> IsLocal() && (vsym -> declarator -> variable_initializer_opt))
						return ResolveType(vsym -> declarator -> variable_initializer_opt -> ExpressionCast());
					return vsym -> Type();
				}
			}
		case Ast::CALL:
			{
			AstMethodInvocation* method_call = expression -> MethodInvocationCast();
			if (method_call -> resolution_opt)
				method_call =  method_call -> resolution_opt -> MethodInvocationCast();
			return  ((MethodSymbol*) method_call -> symbol) -> Type();
			}
		case Ast::CAST:
			return ResolveType(expression -> CastExpressionCast() -> expression);			
		case Ast::PARENTHESIZED_EXPRESSION:
			return ResolveType(expression -> ParenthesizedExpressionCast() -> expression);
		default:
			return NULL;
	}
}

MethodSymbol *DelegationTable::Delegates(TypeSymbol *from, TypeSymbol *to)
{
	assert(from -> ACC_INTERFACE() && (! to -> ACC_INTERFACE()));
	
	unsigned i = 0;	
	while (i < table -> size())
	{
		DelegationEntry *entry = (*table)[i];
		if (((entry -> from == from) || entry -> from -> Implements(from))
		&& ((entry -> to == to) || entry -> to -> IsSubclass(to)))
		{
#ifdef GOF_CONSOLE	
			Coutput << entry -> from -> Utf8Name() 
				<< " delegates " 
				<< entry -> to -> Utf8Name() 
				<< "::"
				<< entry -> method -> Utf8Name()
				<< endl;
#endif
			return entry -> method;
		}
		i++;
	}
	return NULL;
}

void DelegationTable::InsertDelegation(TypeSymbol *from, TypeSymbol *to, AstExpression *base_opt, VariableSymbol *vsym, MethodSymbol *method, MethodSymbol *enclosing, AstMethodInvocation *call)
{
	unsigned i = 0;
	while ((i < table -> size())
		&& (((*table)[i] -> from != from) 
		   ||((*table)[i] -> to != to) 
		   ||((*table)[i] -> base_opt != base_opt)
		   ||((*table)[i] -> vsym != vsym)
		   ||((*table)[i] -> method != method) 
		   ||((*table)[i] -> enclosing != enclosing)
		   ||((*table)[i] -> call != call))) i++;	

	if (i == table -> size())
		table -> push_back(new DelegationEntry(from, to, base_opt, vsym, method, enclosing, call));
}

void DelegationTable::ShowDelegations(TypeSymbol *from, TypeSymbol *to)
{
	if (from -> ACC_INTERFACE() && to -> ACC_INTERFACE())
	{
		for (unsigned i = 0; i < table -> size(); i++)
		{
			DelegationEntry *entry = (*table)[i];
			bool flag1, flag2;

			flag1 = (entry -> from -> ACC_INTERFACE() && entry -> from -> IsSubinterface(from)) 
				? true 
				: (!entry -> from -> ACC_INTERFACE() && entry -> from -> Implements(from))
				? true
				: false;
				
			flag2 = (entry -> to -> ACC_INTERFACE() && entry -> to -> IsSubinterface(to)) 
				? true 
				: (!entry -> to -> ACC_INTERFACE() && entry -> to -> Implements(to))
				? true
				: false;

			if (flag1 && flag2)
			{
				Coutput << entry -> from -> Utf8Name() 
					<< " --> " 
					<< entry -> to -> Utf8Name()
					<< " (";

				if (entry -> base_opt == NULL)
					Coutput << "";
				else if (entry -> base_opt -> kind == Ast::THIS_CALL)
					Coutput << "this";
				else if ((entry -> base_opt -> kind == Ast::CAST) && (entry -> base_opt -> CastExpressionCast() -> expression -> kind == Ast::NAME))
					Coutput << entry -> base_opt -> CastExpressionCast() -> expression -> symbol -> VariableCast() -> Utf8Name();							
				else if ((entry -> base_opt -> kind == Ast::NAME) && (entry -> base_opt -> symbol -> Kind() == Symbol::VARIABLE))
					Coutput << entry -> base_opt -> symbol -> VariableCast() -> Utf8Name();
				else if ((entry -> base_opt -> kind == Ast::NAME) && (entry -> base_opt -> symbol -> Kind() == Symbol::TYPE))
					Coutput << entry -> base_opt -> symbol -> TypeCast() -> Utf8Name();
				else
					Coutput << "unknown";

				Coutput << "."
					<< entry -> method -> Utf8Name()
					<< ")"
					<< endl;
			}
		}
	}
	else if (!from -> ACC_INTERFACE() && to -> ACC_INTERFACE())
	{
		for (unsigned i = 0; i < table -> size(); i++)
		{
			DelegationEntry *entry = (*table)[i];
			bool flag1, flag2;

			flag1 = (!entry -> from -> ACC_INTERFACE() && entry -> from -> IsSubclass(from)) 
				? true 
				: false;
				
			flag2 = (entry -> to -> ACC_INTERFACE() && entry -> to -> IsSubinterface(to)) 
				? true 
				: (!entry -> to -> ACC_INTERFACE() && entry -> to -> Implements(to))
				? true
				: false;



			if (flag1 && flag2)
			{
				Coutput << entry -> from -> Utf8Name() 
					<< " --> " 
					<< entry -> to -> Utf8Name()
					<< " (";

				if (entry -> base_opt)
					Coutput << ResolveType(entry -> base_opt) -> Utf8Name();	
/*
				if (entry -> base_opt == NULL)
					Coutput << "";
				else if (entry -> base_opt -> kind == Ast::THIS_CALL)
					Coutput << "this";
				else if (entry -> base_opt -> kind == Ast::PARENTHESIZED_EXPRESSION)
					Coutput << "(...)";
				else if ((entry -> base_opt -> kind == Ast::CAST) && (entry -> base_opt -> CastExpressionCast() -> expression -> kind == Ast::NAME))
					Coutput << entry -> base_opt -> CastExpressionCast() -> expression -> symbol -> VariableCast() -> Utf8Name();							
				else if ((entry -> base_opt -> kind == Ast::NAME) && (entry -> base_opt -> symbol -> Kind() == Symbol::VARIABLE))
					Coutput << entry -> base_opt -> symbol -> VariableCast() -> Utf8Name();
				else if ((entry -> base_opt -> kind == Ast::NAME) && (entry -> base_opt -> symbol -> Kind() == Symbol::TYPE))
					Coutput << entry -> base_opt -> symbol -> TypeCast() -> Utf8Name();
				else
					Coutput << "unknown";
*/
				Coutput << "."
					<< entry -> method -> Utf8Name()
					<< ")"
					<< endl;
			}
		}
	}
}
void DelegationTable::ConcretizeDelegations()
{
	unsigned i;
	for (i = 0; i<table->size(); i++)
	{
		DelegationEntry *entry = (*table)[i];
		if (entry-> from && entry->vsym && entry->vsym->concrete_types)
		{
			Symbol *sym = entry -> vsym -> concrete_types -> FirstElement();
			while (sym)
			{
				if (!sym -> TypeCast() -> call_dependents)
					sym -> TypeCast() -> call_dependents = new SymbolSet(0);
				sym -> TypeCast() -> call_dependents -> AddElement(entry -> from);
				sym = entry -> vsym -> concrete_types -> NextElement();
			}
		}
	}
}
void DelegationTable::DumpTable()
{
	Coutput << "delegationTable::DumpTable" << endl;
	unsigned i;
	for (i = 0; i < table -> size(); i++)
	{
		DelegationEntry *entry = (*table)[i];
		Coutput << entry -> from -> Utf8Name() 
			<< " --> "
			<< entry -> to -> Utf8Name()
			<< " ("
			<< entry -> method -> Utf8Name()
			<< ") at "
			<< entry -> enclosing -> Utf8Name()
			<< endl
			<< endl;
	}
}

int ClassSymbolTable::ConcreteClasses()
{
	int ct = 0;

	for (unsigned c = 0; c < table->size(); c++)
	{
		if ((!(*table)[c] -> ACC_INTERFACE())
		&& (!(*table)[c] -> ACC_ABSTRACT())
		&& (!(*table)[c] -> ACC_SYNTHETIC())
		&& (!(*table)[c] ->IsInner()))
			ct++;
	}
	return ct;
}

bool ClassSymbolTable::Converge(TypeSymbol* super1, TypeSymbol* super2)
{
	bool flag1, flag2;
	flag1 = flag2 = false;

	for (unsigned c = 0; (!flag1 ||!flag2) && (c < table->size()); c++)
	{
		TypeSymbol *type = (*table)[c];
		if (type -> ACC_INTERFACE())
		{
			if (super1 -> ACC_INTERFACE())
				flag1 = type -> IsSubinterface(super1);
			else
				flag1 = false;

			if (super2 -> ACC_INTERFACE())
				flag2 = type -> IsSubinterface(super2);
			else
				flag2 = false;
		}
		else
		{
			if (super1 -> ACC_INTERFACE())
				flag1 = type -> Implements(super1);
			else
				flag1 = type -> IsSubclass(super1);

		
			if (super2 -> ACC_INTERFACE())
				flag2 = type -> Implements(super2);
			else
				flag2 = type -> IsSubclass(super2);
		}
	}

#ifdef GOF_CONSOLE	
	if (flag1 && flag2)
		Coutput << (--p) -> first << " converges " << super1->Utf8Name() << " and " << super2->Utf8Name() << endl;
#endif

	return (flag1 && flag2);
}

void ClassSymbolTable::AddClassSymbol(TypeSymbol *sym)
{
	table->push_back(sym);	
}

TypeSymbol *ClassSymbolTable::GetSymbol(wchar_t * cls)
{
	for (unsigned c = 0; c < table->size(); c++)
	{
		TypeSymbol *sym = (*table)[c];
		if (wcscmp(cls, sym-> Name()) == 0)
			return sym;			
	}
	return NULL;
}

bool ClassSymbolTable::IsFamily(TypeSymbol* t1, TypeSymbol *t2)
{
	if ((t1 -> ACC_INTERFACE() ||t1 -> ACC_ABSTRACT() || t1 -> ACC_SYNTHETIC())
	|| (t2 -> ACC_INTERFACE() ||t2 -> ACC_ABSTRACT() || t2 -> ACC_SYNTHETIC()))
		return false;

	for (unsigned c = 0; c < table->size(); c++)
	{
		TypeSymbol *type = (*table)[c];
		if (type->ACC_SYNTHETIC())
		{
			if (type -> ACC_INTERFACE())
				return (t1->Implements(type) && t2->Implements(type));
			else
				return (t1->IsSubclass(type) && t2->IsSubclass(type));
			
		}		
	}
	return false;
}

bool ClassSymbolTable::HasSubclasses(TypeSymbol *super)
{
	if (!super -> ACC_INTERFACE())
	{
		for (unsigned c = 0; c < table->size(); c++)
		{
			TypeSymbol *type = (*table)[c];
			if (type -> IsSubclass(super))
				return true;
		}
		return false;
	}
	return false;
}

vector<TypeSymbol*> *ClassSymbolTable::GetAncestors(TypeSymbol *type)
{
// Error: includes Object, which every class subclasses
	vector<TypeSymbol*> *a = new vector<TypeSymbol*>();

       for (; type; type = type -> super)
		a -> push_back(type);
	return a;
}

void ClassSymbolTable::PrintSubclasses(TypeSymbol* super)
{
	assert (! super -> ACC_INTERFACE());
	
	for (unsigned c = 0; c < table->size(); c++)
	{
		if ((!(*table)[c]->ACC_INTERFACE())
		&& ((*table)[c] -> IsSubclass(super)))
			Coutput << (*table)[c]->Utf8Name() << " ";
	}
	Coutput << endl;
}

void ClassSymbolTable::PrintSubinterfaces(TypeSymbol* inter)
{
	assert (inter -> ACC_INTERFACE());
	
	for (unsigned c = 0; c < table->size(); c++)
	{
		if (((*table)[c]->ACC_INTERFACE())
		&& ((*table)[c]->IsSubinterface(inter)))
			Coutput << (*table)[c]->Utf8Name() << " ";
	}
	Coutput << endl;
}

void ClassSymbolTable::PrintSubtypes(TypeSymbol *inter)
{
	assert (inter -> ACC_INTERFACE());
	
	for (unsigned c = 0; c < table->size(); c++)
	{
		if ((!(*table)[c]->ACC_INTERFACE()) && ((*table)[c]->Implements(inter)))
		{
			Coutput << (*table)[c]->Utf8Name();
			if ((*table)[c]->IsInner())
				Coutput << "(inner) ";
			else if ((*table)[c]->ACC_PRIVATE())
				Coutput << "(private) ";				
			else
				Coutput  << " ";
		}
	}
	Coutput << endl;
}

void ClassSymbolTable::ExpandSubtypes()
{
	unsigned p, q;
	for (p = 0; p < table->size(); p++)
	{
	/*
		Coutput << p->second->fully_qualified_name->value  << ": " << endl;
		if (p->second->subtypes_closure && p->second->subtypes_closure->Size())
		{
			Coutput << "subtypes_closure: ";
			p->second->subtypes_closure->Print();
			
		}
		if (p->second->subtypes && p->second->subtypes->Size())
		{
			Coutput << "subtypes: ";
			p->second->subtypes->Print();
			
		}
		Coutput << endl;
	*/
		if ((*table)[p]->subtypes)
		{
			for (q = 0; q < table->size(); q++)
			{
				if ((*table)[p] != (*table)[q]
				&& !(*table)[p]->subtypes->IsElement((*table)[q])
				&& (*table)[q]->IsSubtype((*table)[p])
				)
					(*table)[p]->subtypes->AddElement((*table)[q]);			
			}
		}
	}	
}

void MethodSymbolTable::AddMethodSymbol(MethodSymbol *sym)
{
	table -> push_back(sym);
}

MethodSymbol *MethodSymbolTable::GetSymbol(char *cls, char *mtd, char *fname)
{
	for (unsigned i=0; i<table->size(); i++)
	{
              MethodSymbol *method_symbol = (*table)[i];
		TypeSymbol *unit_type = method_symbol -> containing_type;

		if ((strcmp(method_symbol->Utf8Name(), mtd) == 0)
		&& (strcmp(unit_type -> Utf8Name(), cls) == 0)
		&& (strcmp(unit_type -> file_symbol -> FileName(), fname) == 0))
			return method_symbol;			
	}
	return NULL;
}

Ast *MethodSymbolTable::GetAstDeclaration(wchar_t *pkg, wchar_t *cls, wchar_t *mtd)
{
	for (unsigned i=0; i<table->size(); i++)
	{
              MethodSymbol *method_symbol = (*table)[i];
		TypeSymbol *unit_type = method_symbol -> containing_type;

		if ((wcscmp(method_symbol -> Name(), mtd) == 0)
		&& (wcscmp(unit_type -> Name(), cls) == 0)
		&& (wcscmp(unit_type -> FileLoc(), pkg) == 0))
			return method_symbol -> declaration;			
	}
	return NULL;
}

void MethodSymbolTable::PrintDeclaration(char *cls, char *mtd, char *fname)
{
	for (unsigned i=0; i<table->size(); i++)
	{
		TypeSymbol *unit_type = (*table)[i]->containing_type;
		if ((strcmp((*table)[i]->Utf8Name(), mtd) == 0)
		&& (strcmp(unit_type -> Utf8Name(), cls) == 0)
		&& (strcmp(unit_type -> file_symbol -> FileName(), fname) == 0))
		{
	       	MethodSymbol *method_symbol = (*table)[i];
			TypeSymbol *type = method_symbol -> containing_type;
			Coutput << "file name: " << type -> file_symbol -> FileName() << endl;
			Coutput << "class name: " << type -> Utf8Name() << endl;		
			method_symbol -> declaration -> Print();
			Coutput << mtd << ": " << dynamic_cast<AstMethodDeclaration*>(method_symbol -> declaration) -> method_body_opt -> NumStatements() << endl;
		}
	}		
}

void MethodSymbolTable::PrintBody(char *cls, char *mtd, char *fname)
{
	unsigned i=0;
	for (; i<table->size(); i++)
	{
		TypeSymbol *unit_type = (*table)[i]->containing_type;
		if ((strcmp((*table)[i]->Utf8Name(), mtd) == 0)
		&& (strcmp(unit_type -> Utf8Name(), cls) == 0)
		&& (strcmp(unit_type -> file_symbol -> FileName(), fname) == 0))
			break;
	}

	if (i<table->size())
	{
	       MethodSymbol *method_symbol = (*table)[i];
		dynamic_cast<AstMethodDeclaration*>(method_symbol -> declaration) -> method_body_opt -> Print();
	}		
}

void MethodSymbolTable::ExpandCallDependents()
{
	// does NOT expand typesymbol->call_dependents
	
	for (unsigned i=0; i<table->size(); i++)
	{
		MethodSymbol *method = (*table)[i];
		MethodSymbol *overridden = method->GetVirtual(); // TODO: consider mult-inheritance
		if (overridden)
		{
			if (overridden->invokers)
			{
				if (!method->invokers)
					method->invokers = new SymbolSet(0);
				method->invokers->Union(*overridden->invokers);

				Symbol *sym = overridden->invokers->FirstElement();
				while(sym)
				{
					MethodSymbol *msym = sym->MethodCast();
					msym->invokees->AddElement(method);
					sym = overridden->invokers->NextElement();
				}				
			}
			
			if (overridden ->callers)
			{
				if (!method->callers)
					method->callers = new SymbolSet(0);
				method->callers->Union(*overridden->callers);
			}
		}
	}	
}

void MethodSymbolTable::ClearMarks()
{
	for (unsigned i=0; i<table->size(); i++)
	{
		if ((*table)[i]->mark != 'W')
			(*table)[i]->mark = 'W';
	}	
}

Env::State EnvTable::getState(wchar_t* var)
{
	unsigned i = 0;
	while ((i < table -> size()) 
	&& (wcscmp((*table)[i] -> var, var) != 0))
		i++;
	return (*table)[i] -> state;
}

void EnvTable::changeState(wchar_t* var, Env::State state)
{
	unsigned i = 0;
	while ((i < table -> size()) 
	&& (wcscmp((*table)[i] -> var, var) != 0))
		i++;

	if (i < table -> size())
		(*table)[i] -> state = state;
}

AstDeclared* MethodBodyTable::getAstLocation(wchar_t* cls, wchar_t* mtd)
{
	MethodBodyAddr*  entry = NULL;
	bool flag = false;
	unsigned i = 0;
	while (!flag && (i < table -> size()))
	{
		entry = (*table)[i];

		if ((wcscmp(cls, entry -> class_name) == 0)
		&& (wcscmp(mtd, entry -> method_name) == 0))
			flag = true;
		else
			i++;
	}
	return (flag) ? entry -> ast_location : NULL;
}

vector<wchar_t*>* MethodBodyTable::getModifiersAt(int i)
{
	AstDeclared *decl = (*table)[i] -> ast_location;

       vector<wchar_t*>* modifiers = NULL;
	if (decl -> modifiers_opt)
	{
		modifiers = new vector<wchar_t*>();
		unsigned i;
		for (i = 0; i < decl -> modifiers_opt -> NumModifiers(); i++)
		{
			if (decl -> modifiers_opt -> Modifier(i) -> kind == Ast::MODIFIER_KEYWORD)
				modifiers -> push_back(dynamic_cast<AstModifierKeyword*>(decl -> modifiers_opt -> Modifier(i)) -> modifier_token_string);
			else
				decl -> Print();
		}
	}

	return modifiers;
}

vector<wchar_t*>* GenTable::getAncestors(GenTable::Kind kind, wchar_t* cls, wchar_t* pkg)
{
	bool flag = false;
	unsigned i = 0;
	while (!flag && (i < table -> size()))
	{
		Gen* entry = (*table)[i];
		if (wcscmp((entry -> class_name), cls) == 0)
		//*&& (wcscmp((entry -> package_name), pkg) == 0))
			flag = true;
		else
			i++;			
	}

	Gen* entry = (*table)[i];

	if (flag && (kind == GenTable::SUBC) && (entry -> kind != Gen::INTERFACE))
	{
		wchar_t* spr = entry -> super_name;
		vector<wchar_t*>* ancestors = new vector<wchar_t*>();
		ancestors -> push_back(cls);

		while (spr)
		{
			ancestors -> push_back(spr);
			spr = getSuper(spr, pkg);
		}
		return ancestors;
	}
	else if (flag && (kind == GenTable::IMPL))
	{
		vector<wchar_t*>* ancestors = new vector<wchar_t*>();
		if ((entry -> kind != Gen::INTERFACE) && (entry -> interfaces))
		{
			unsigned j;
			for (j = 0; j < entry -> interfaces -> size(); j++)			
				ancestors -> push_back((*entry -> interfaces)[j]);
		}

		if (entry -> kind == Gen::INTERFACE)
			ancestors -> push_back(entry -> class_name);
		return ancestors;			
	}
	return NULL;
}

wchar_t* GenTable::getSuper(wchar_t* cls, wchar_t* pkg)
{	
	unsigned i = 0;
	Gen*entry = NULL;
	while (!entry && (i < table -> size()))
	{		
		if (wcscmp(((*table)[i] -> class_name), cls) == 0)
		{
			if ((((*table)[i] -> package_name == NULL) && (pkg == NULL))
			|| (((*table)[i] -> package_name != NULL) && (pkg != NULL) && (wcscmp((*table)[i] -> package_name, pkg) == 0)))
				entry = (*table)[i];
			else
				i++;
		}
		else
			i++;			
	}
	return (entry)?entry -> super_name:NULL;
}

vector<wchar_t*>* GenTable::getSuccessors(const wchar_t* super, GenTable::Kind kind)
{
	vector<wchar_t*>* list = NULL;
	unsigned i;
	if (kind == GenTable::IMPL)
	{		
		for  (i = 0; i < table -> size(); i++)
		{
			Gen* entry = (*table)[i];
			unsigned j;
			if (entry -> interfaces)
			{
				for (j = 0; j < entry -> interfaces -> size(); j++)
				{
					if (wcscmp(super, (*entry -> interfaces)[j]) == 0)
					{
						if (list == NULL)
							list = new vector<wchar_t*>();
						list -> push_back(entry -> class_name);
					}
				}
			}
		}
	}
	else
	{
		for  (i = 0; i < table -> size(); i++)
		{
			Gen* entry = (*table)[i];
			//unsigned j;
			if (entry -> super_name)
			{
				if (wcscmp(super, entry -> super_name) == 0)
				{
						if (list == NULL)
							list = new vector<wchar_t*>();
						list -> push_back(entry -> class_name);
				}
			}
		}	
	}
	return list;
}

Gen::Kind GenTable::getKind(wchar_t* cls, wchar_t* pkg)
{
	Gen* entry = NULL;
	bool flag = false;
	unsigned i = 0;
	while (!flag && (i < table -> size()))
	{
		entry = (*table)[i];

		if (wcscmp(entry -> class_name, cls) == 0)
		{
			if (((entry -> package_name == NULL) && (pkg == NULL))
			|| (((entry -> package_name != NULL) && (pkg != NULL)) && (wcscmp(entry -> package_name, pkg) == 0)))
				flag = true;
			else
				i++;
		}
		else
			i++;
	}
	assert(flag);
	return entry ->kind;
}

char* GenTable::getFileName(wchar_t* cls, wchar_t* pkg)
{
	Gen* entry = NULL;
	bool flag = false;
	unsigned i = 0;
	while (!flag && (i < table -> size()))
	{
		entry = (*table)[i];

		if (wcscmp(entry -> class_name, cls) == 0)
		{
			if (((entry -> package_name == NULL) && (pkg == NULL))
			|| ((entry -> package_name != NULL && (pkg != NULL)) && (wcscmp(entry -> package_name, pkg) == 0)))
				flag = true;
			else
				i++;
		}
		else
			i++;
	}
	return (flag) ? entry ->file_name : NULL;
}

vector<wchar_t*>* GenTable::getInterfaces(wchar_t* pkg, wchar_t* cls)
{
	Gen* entry = NULL;
	bool flag = false;
	unsigned i = 0;
	while (!flag && (i < table -> size()))
	{
		entry = (*table)[i];

		if ((wcscmp(entry -> package_name, pkg) == 0)
		&& (wcscmp(entry -> class_name, cls) == 0))
		{
			flag = true;
		}
		else
			i++;
	}
	return (flag) ? entry -> interfaces : NULL;
}

wchar_t* AssocTable::getName(Assoc::Kind kind, Assoc::Mode mode, wchar_t* type, wchar_t* cls)
{
	Assoc*  entry = NULL;
	bool flag = false;
	unsigned i = 0;
	while (!flag && (i < table -> size()))
	{
		entry = (*table)[i];

		if ((entry -> kind == kind)
		&& (entry -> mode == mode)
		&& (wcscmp(entry -> type, type) == 0)
		&& (wcscmp(entry -> class_name, cls) == 0))
			flag = true;
		else
			i++;
	}
	return (flag) ? entry -> name : NULL;	
}

wchar_t* AssocTable::getType(Assoc::Kind kind, Assoc::Mode mode, wchar_t* name, wchar_t* pkg, wchar_t* cls)
{
	Assoc*  entry = NULL;
	bool flag = false;
	unsigned i = 0;
	while (!flag && (i < table -> size()))
	{
		entry = (*table)[i];

		if ((entry -> kind == kind)
		&& (entry -> mode == mode)
		&& (wcscmp(entry -> name, name) == 0)
//		&& (wcscmp(entry -> package_name, pkg) == 0)
		&& (wcscmp(entry -> class_name, cls) == 0))
			flag = true;
		else
			i++;
	}
	return (flag) ? entry -> type : NULL;	
}

bool AssocTable::isInvoked(wchar_t* name, wchar_t* cls)
{
	unsigned i = 0;
	while (i < table -> size())
	{
		Assoc* entry = (*table)[i];
		if ((entry -> kind == Assoc::MI)
		&& (entry -> type)
		&& (wcscmp(entry -> type, name) == 0)
		&& (wcscmp(entry -> class_name, cls) == 0))
			return true;
		i++;
	}
	return false;
}

void EnvTable::addEnvironment(wchar_t* var, Env::State state)
{
	Env* entry = new Env(var, state);
	table -> push_back(entry);
}

void MethodBodyTable::addMethodBodyAddr(wchar_t* pkg, wchar_t* cls, wchar_t* mtd, AstDeclared* ptr)
{
	MethodBodyAddr* entry = new MethodBodyAddr(pkg, cls, mtd, ptr);
	table -> push_back(entry);
}

void GenTable::addGeneralization(wchar_t* pkg, wchar_t* cls, wchar_t* spr, vector<wchar_t*>* ifcs, Gen::Kind k, char* f)
{
	Gen* entry = new Gen(pkg, cls, spr, ifcs, k, f);
	table -> push_back(entry);
}

void AssocTable::addAssociation(Assoc::Kind kind, Assoc::Mode mode, wchar_t* name, wchar_t* type, wchar_t* pkg, wchar_t* cls, wchar_t* mtd)
{
	Assoc* entry = new Assoc(kind, mode, name, type, pkg, cls, mtd);
	table -> push_back(entry);
}

void MethodBodyTable::dumpTable()
{
	Coutput << L"Method Body Reference Table" << endl;
	unsigned i;
	for (i = 0; i < table -> size(); i++)
	{
		MethodBodyAddr* entry = (*table)[i];
		Coutput << entry -> class_name 
			     << "::"
			     << entry -> method_name 
			     << endl;

		if (entry -> ast_location -> kind == Ast::CONSTRUCTOR)
		{
			Coutput << L"***constructor***" << endl;
			dynamic_cast<AstConstructorDeclaration*>(entry -> ast_location) -> Print();			
		}
		else
		{
			Coutput << L"***method***" << endl;
			dynamic_cast<AstMethodDeclaration*>(entry -> ast_location) -> Print();
		}
	}
	Coutput << endl;
}

void GenTable::dumpTable()
{
       Coutput << L"Generalization Table" << endl;
	unsigned i;
	for (i = 0; i < table -> size(); i++)
	{
		Gen* entry = (*table)[i];
		if (entry -> kind == Gen::CLASS)
		{
			Coutput << entry -> class_name << " |--> " 
			 	     << ((entry -> super_name) ? entry -> super_name : L"java.lang.Object")
			     	     << " ";

			unsigned j;
			Coutput << "{";
			if (entry -> interfaces)
			{
				for (j = 0; j < entry -> interfaces -> size(); j++)
				{
					unsigned end = entry -> interfaces -> size() - 1;

					Coutput << (*(entry -> interfaces))[j];
			
					if (j < end)
						Coutput << ", ";
				}
			}
			Coutput << "}" << endl;
		}
		else
		{
			Coutput << entry -> class_name << "  " ;
			unsigned j;
			Coutput << "{";
			if (entry -> interfaces)
			{
				for (j = 0; j < entry -> interfaces -> size(); j++)
				{
					unsigned end = entry -> interfaces -> size() - 1;

					Coutput << (*(entry -> interfaces))[j];
			
					if (j < end)
						Coutput << ", ";
				}
			}
			Coutput << "} " << endl;
		}
		Coutput << entry -> package_name << endl << entry -> file_name << endl;
	}
}

void AssocTable::dumpTable()
{
       Coutput << L"Association Table" << endl;
	unsigned i;
	for (i = 0; i < table -> size(); i++)
	{
		Assoc* entry = (*table)[i];
		Coutput << entry -> class_name << " --> "
			     << entry -> type;

              Coutput << " (by ";
		switch(entry -> kind)
		{
			case Assoc::CF:
				if (entry -> mode == Assoc::PRIVATE)
					Coutput << L"private class member " << entry -> name << ")";
				else if (entry -> mode == Assoc::PROTECTED)
					Coutput << L"protected class member " << entry -> name << ")";
				else
					Coutput << L"public class member " << entry -> name << ")";
				break;
			case Assoc::IM:
				Coutput << L"instance member " << entry -> name << ")";
				break;
			case Assoc::MP:
				Coutput << L"parameter " << entry -> name << L" in " << entry -> method_name << "(...))";
				break;
			case Assoc::OC:
				break;
			case Assoc::MR:
				if (entry -> mode == Assoc::PRIVATE)
					Coutput << L"return type in private instance method " << entry -> method_name << "(...))";
				else if (entry -> mode == Assoc::PROTECTED)
					Coutput << L"return type in protected instance method " << entry -> method_name << "(...))";
				else
					Coutput << L"return type in public instance method " << entry -> method_name << "(...))";
				break;
			case Assoc::CM:
				if (entry -> mode == Assoc::PRIVATE)
					Coutput << L"return type in private static method " << entry -> method_name << "(...))";
				else if (entry -> mode == Assoc::PROTECTED)
					Coutput << L"return type in protected static method " << entry -> method_name << "(...))";
				else
					Coutput << L"return type in public static method " << entry -> method_name << "(...))";
				break;
			default:
				break;
		}
		Coutput << endl;
	}
	Coutput << endl;
}

void Statechart::Print()
{
	unsigned i;
	for (i = 0; i < statechart -> size(); i++)
	{
		switch((*statechart)[i] -> kind)
		{
			case State::GET:
				Coutput << "GET state.";
				break;
			case State::SET:
				Coutput << "SET state.";
				break;
			case State::CREATE:
				Coutput << "CREATE state.";
				break;
			case State::RETURN:
				Coutput << "RETURN state.";
				break;
			case State::CONDITION:
				Coutput << "CONDITION state.";
				break;
			default:
				break;			
		}
		Coutput << endl;

		if ((*statechart)[i] -> kind != State::RETURN)
		{
			unsigned j;
			for (j = 0; j < (*statechart)[i] -> participants -> size(); j++)
				Coutput << " " << (*(*statechart)[i] -> participants)[j];
			Coutput << endl;
		}
	}
}


void visit(AstConditionalExpression *cond_expression)
{
	// TODO: should add conditional BLOCK statements 
}

Control::Control(Option& option_)
    : return_code(0)
    , option(option_)
    , dot_classpath_index(0)
    , system_table(NULL)
    , system_semantic(NULL)
    , semantic(1024)
    , needs_body_work(1024)
    , type_trash_bin(1024)
    , input_java_file_set(1021)
    , input_class_file_set(1021)
    , expired_file_set()
    , recompilation_file_set(1021)
    // Type and method cache. These variables are assigned in control.h
    // accessors, but must be NULL at startup.
    , Annotation_type(NULL)
    , AssertionError_type(NULL)
    , AssertionError_Init_method(NULL)
    , AssertionError_InitWithChar_method(NULL)
    , AssertionError_InitWithBoolean_method(NULL)
    , AssertionError_InitWithInt_method(NULL)
    , AssertionError_InitWithLong_method(NULL)
    , AssertionError_InitWithFloat_method(NULL)
    , AssertionError_InitWithDouble_method(NULL)
    , AssertionError_InitWithObject_method(NULL)
    , Boolean_type(NULL)
    , Boolean_TYPE_field(NULL)
    , Byte_type(NULL)
    , Byte_TYPE_field(NULL)
    , Character_type(NULL)
    , Character_TYPE_field(NULL)
    , Class_type(NULL)
    , Class_forName_method(NULL)
    , Class_getComponentType_method(NULL)
    , Class_desiredAssertionStatus_method(NULL)
    , ClassNotFoundException_type(NULL)
    , Cloneable_type(NULL)
    , Comparable_type(NULL)
    , Double_type(NULL)
    , Double_TYPE_field(NULL)
    , ElementType_type(NULL)
    , ElementType_TYPE_field(NULL)
    , ElementType_FIELD_field(NULL)
    , ElementType_METHOD_field(NULL)
    , ElementType_PARAMETER_field(NULL)
    , ElementType_CONSTRUCTOR_field(NULL)
    , ElementType_LOCAL_VARIABLE_field(NULL)
    , ElementType_ANNOTATION_TYPE_field(NULL)
    , ElementType_PACKAGE_field(NULL)
    , Enum_type(NULL)
    , Enum_Init_method(NULL)
    , Enum_ordinal_method(NULL)
    , Enum_valueOf_method(NULL)
    , Error_type(NULL)
    , Exception_type(NULL)
    , Float_type(NULL)
    , Float_TYPE_field(NULL)
    , Integer_type(NULL)
    , Integer_TYPE_field(NULL)
    , Iterable_type(NULL)
    , Iterable_iterator_method(NULL)
    , Iterator_type(NULL)
    , Iterator_hasNext_method(NULL)
    , Iterator_next_method(NULL)
    , Long_type(NULL)
    , Long_TYPE_field(NULL)
    , NoClassDefFoundError_type(NULL)
    , NoClassDefFoundError_Init_method(NULL)
    , NoClassDefFoundError_InitString_method(NULL)
    , Object_type(NULL)
    , Object_getClass_method(NULL)
    , Overrides_type(NULL)
    , Retention_type(NULL)
    , RetentionPolicy_type(NULL)
    , RetentionPolicy_SOURCE_field(NULL)
    , RetentionPolicy_CLASS_field(NULL)
    , RetentionPolicy_RUNTIME_field(NULL)
    , RuntimeException_type(NULL)
    , Serializable_type(NULL)
    , Short_type(NULL)
    , Short_TYPE_field(NULL)
    , String_type(NULL)
    , StringBuffer_type(NULL)
    , StringBuffer_Init_method(NULL)
    , StringBuffer_InitWithString_method(NULL)
    , StringBuffer_toString_method(NULL)
    , StringBuffer_append_char_method(NULL)
    , StringBuffer_append_boolean_method(NULL)
    , StringBuffer_append_int_method(NULL)
    , StringBuffer_append_long_method(NULL)
    , StringBuffer_append_float_method(NULL)
    , StringBuffer_append_double_method(NULL)
    , StringBuffer_append_string_method(NULL)
    , StringBuffer_append_object_method(NULL)
    , StringBuilder_type(NULL)
    , StringBuilder_Init_method(NULL)
    , StringBuilder_InitWithString_method(NULL)
    , StringBuilder_toString_method(NULL)
    , StringBuilder_append_char_method(NULL)
    , StringBuilder_append_boolean_method(NULL)
    , StringBuilder_append_int_method(NULL)
    , StringBuilder_append_long_method(NULL)
    , StringBuilder_append_float_method(NULL)
    , StringBuilder_append_double_method(NULL)
    , StringBuilder_append_string_method(NULL)
    , StringBuilder_append_object_method(NULL)
    , Target_type(NULL)
    , Throwable_type(NULL)
    , Throwable_getMessage_method(NULL)
    , Throwable_initCause_method(NULL)
    , Void_type(NULL)
    , Void_TYPE_field(NULL)
    // storage for all literals seen in source
    , int_pool(&bad_value)
    , long_pool(&bad_value)
    , float_pool(&bad_value)
    , double_pool(&bad_value)
    , Utf8_pool(&bad_value)
#ifdef JIKES_DEBUG
    , input_files_processed(0)
    , class_files_read(0)
    , class_files_written(0)
    , line_count(0)   
#endif // JIKES_DEBUG
    // Package cache.  unnamed and lang are initialized in constructor body.
    , annotation_package(NULL)
    , io_package(NULL)
    , util_package(NULL) {
    PINOT_DEBUG = (getenv("PINOT_DEBUG")) != nullptr;
    option.bytecode = false;

// breakpoint 0.
// getchar();
    r_table = new ReadAccessTable();
    w_table = new WriteAccessTable();
    d_table = new DelegationTable();
    cs_table = new ClassSymbolTable();
    ms_table = new MethodSymbolTable();

    nSingleton = nCoR = nBridge = nStrategy = nState = nFlyweight = nComposite = nMediator = nTemplate = nFactoryMethod = nAbstractFactory = nVisitor = nDecorator = nObserver = nProxy = nAdapter = nFacade = 0;


    mb_table = new MethodBodyTable();
    gen_table = new GenTable();
    assoc_table = new AssocTable();

    counter1 = 0;
    counter2 = 0;
    counter3 = 0;

    ProcessGlobals();
    ProcessUnnamedPackage();
    ProcessPath();
    ProcessSystemInformation();

    //
    // Instantiate a scanner and a parser and initialize the static members
    // for the semantic processors.
    //
    scanner = new Scanner(*this);
    parser = new Parser();
    SemanticError::StaticInitializer();

    ast_pool = new StoragePool(64);
}

RunStats Control::run(char **arguments) {
    //
    // Process all file names specified in command line
    //
    ProcessNewInputFiles(input_java_file_set, arguments);

    //
    // For each input file, copy it into the input_files array and process
    // its package declaration. Estimate we need 64 tokens.
    //
    FileSymbol **input_files = new FileSymbol *[input_java_file_set.Size() + 1];
    int num_files = 0;
    FileSymbol *file_symbol;
    for (file_symbol = (FileSymbol *) input_java_file_set.FirstElement();
         file_symbol;
         file_symbol = (FileSymbol *) input_java_file_set.NextElement()) {
        input_files[num_files++] = file_symbol;

#ifdef JIKES_DEBUG
        input_files_processed++;
#endif
        errno = 0;
        scanner->Scan(file_symbol);
        if (file_symbol->lex_stream) // did we have a successful scan!
        {
            AstPackageDeclaration *package_declaration =
                    parser->PackageHeaderParse(file_symbol->lex_stream,
                                               ast_pool);

            ProcessPackageDeclaration(file_symbol, package_declaration);
            ast_pool->Reset();
        } else {
            const char *std_err = strerror(errno);
            ErrorString err_str;
            err_str << '"' << std_err << '"' << " while trying to open "
                    << file_symbol->FileName();
            general_io_errors.Next() = err_str.SafeArray();
        }
    }

    //
    //
    //
    FileSymbol *main_file_clone;
    if (num_files > 0)
        main_file_clone = input_files[0]->Clone();
    else {
        //
        // Some name, any name !!! We use dot_name_symbol as a bad file name
        // because no file can be named ".".
        //
        FileSymbol *file_symbol = classpath[dot_classpath_index]->
                RootDirectory()->InsertFileSymbol(dot_name_symbol);
        file_symbol->directory_symbol = classpath[dot_classpath_index]->
                RootDirectory();
        file_symbol->SetJava();

        main_file_clone = file_symbol->Clone();
    }

    main_file_clone->semantic = new Semantic(*this, main_file_clone);
    system_semantic = main_file_clone->semantic;
    scanner->SetUp(main_file_clone);

#ifdef WIN32_FILE_SYSTEM
    //
    //
    //
    if (option.BadMainDisk())
    {
        system_semantic -> ReportSemError(SemanticError::NO_CURRENT_DIRECTORY,
                                          BAD_TOKEN);
    }
#endif // WIN32_FILE_SYSTEM

    unsigned i;
    for (i = 0; i < bad_dirnames.Length(); i++) {
        system_semantic->
                ReportSemError(SemanticError::CANNOT_OPEN_PATH_DIRECTORY,
                               BAD_TOKEN, bad_dirnames[i]);
    }
    for (i = 0; i < bad_zip_filenames.Length(); i++) {
        system_semantic->ReportSemError(SemanticError::CANNOT_OPEN_ZIP_FILE,
                                        BAD_TOKEN, bad_zip_filenames[i]);
    }
    for (i = 0; i < general_io_warnings.Length(); i++) {
        system_semantic->ReportSemError(SemanticError::IO_WARNING, BAD_TOKEN,
                                        general_io_warnings[i]);
        delete[] general_io_warnings[i];
    }
    for (i = 0; i < general_io_errors.Length(); i++) {
        system_semantic->ReportSemError(SemanticError::IO_ERROR, BAD_TOKEN,
                                        general_io_errors[i]);
        delete[] general_io_errors[i];
    }

    //
    // Require the existence of java.lang.
    //
    if (lang_package->directory.Length() == 0) {
        system_semantic->ReportSemError(SemanticError::PACKAGE_NOT_FOUND,
                                        BAD_TOKEN,
                                        StringConstant::US_java_SL_lang);
    }

    //
    // When the -d option is specified, create the relevant
    // directories if they don't already exist.
    //
    if (option.directory) {
        if (!SystemIsDirectory(option.directory)) {
            for (char *ptr = option.directory; *ptr; ptr++) {
                char delimiter = *ptr;
                if (delimiter == U_SLASH) {
                    *ptr = U_NULL;

                    if (!SystemIsDirectory(option.directory))
                        SystemMkdir(option.directory);

                    *ptr = delimiter;
                }
            }

            SystemMkdir(option.directory);

            if (!SystemIsDirectory(option.directory)) {
                int length = strlen(option.directory);
                wchar_t *name = new wchar_t[length + 1];
                for (int j = 0; j < length; j++)
                    name[j] = option.directory[j];
                name[length] = U_NULL;
                system_semantic->ReportSemError(SemanticError::CANNOT_OPEN_DIRECTORY,
                                                BAD_TOKEN, name);
                delete[] name;
            }
        }
    }

    //
    //
    //
    for (i = 0; i < bad_input_filenames.Length(); i++) {
        system_semantic->ReportSemError(SemanticError::BAD_INPUT_FILE,
                                        BAD_TOKEN, bad_input_filenames[i]);
    }

    //
    //
    //
    for (i = 0; i < unreadable_input_filenames.Length(); i++) {
        system_semantic->ReportSemError(SemanticError::UNREADABLE_INPUT_FILE,
                                        BAD_TOKEN,
                                        unreadable_input_filenames[i]);
    }

    //
    //
    //
    if (system_semantic->NumErrors() > 0) {
        system_semantic->PrintMessages();
        return_code = system_semantic->return_code;
    } else {
        //
        // There might be some warnings we want to print.
        //
        system_semantic->PrintMessages();
        input_java_file_set.SetEmpty();
        for (int j = 0; j < num_files; j++) {
            FileSymbol *file_symbol = input_files[j];
            if (!input_java_file_set.IsElement(file_symbol))
                ProcessFile(file_symbol, ast_pool);
        }

        d_table->ConcretizeDelegations();
        ms_table->ExpandCallDependents();
        cs_table->ExpandSubtypes();
    }

    delete main_file_clone; // delete the clone of the main source file...
    delete[] input_files;

    return {
            return_code,
            gen_table->getSize(),
            num_files,
            d_table->size(),
            cs_table->ConcreteClasses(),
            d_table->UniqueDirectedCalls()
    };
}

Control::~Control()
{
    //
    // Clean up all the files that have just been compiled in this new
    // batch.
    //
    FileSymbol* file_symbol;
    for (file_symbol = (FileSymbol*) input_java_file_set.FirstElement();
         file_symbol;
         file_symbol = (FileSymbol*) input_java_file_set.NextElement())
    {
        CleanUp(file_symbol);
    }

    //
    // If more messages were added to system_semantic, print them...
    //
    system_semantic -> PrintMessages();
    if (system_semantic -> return_code > 0 ||
        bad_input_filenames.Length() > 0 ||
        unreadable_input_filenames.Length() > 0)
    {
        return_code = 1;
    }

    unsigned i;
    for (i = 0; i < bad_zip_filenames.Length(); i++) delete [] bad_zip_filenames[i];
    for (i = 0; i < bad_input_filenames.Length(); i++) delete [] bad_input_filenames[i];
    for (i = 0; i < unreadable_input_filenames.Length(); i++) delete [] unreadable_input_filenames[i];
    for (i = 0; i < system_directories.Length(); i++) delete system_directories[i];
    /*for (i = 0; i < bad_dirnames.Length(); i++) delete [] bad_dirnames[i];
    for (i = 0; i < semantic.Length(); i++) delete semantic[i];
    for (i = 0; i < needs_body_work.Length(); i++) delete needs_body_work[i];
    for (i = 0; i < type_trash_bin.Length(); i++) delete type_trash_bin[i];*/

    delete scanner;
    delete parser;
    delete system_semantic;
    delete system_table;

    /*delete access_name_symbol;
    delete array_name_symbol;
    delete assert_name_symbol;
    delete block_init_name_symbol;
    delete class_name_symbol;
    delete clinit_name_symbol;
    delete clone_name_symbol;
    delete dot_name_symbol;
    delete dot_dot_name_symbol;
    delete Enum_name_symbol;
    delete equals_name_symbol;
    delete false_name_symbol;
    delete hashCode_name_symbol;
    delete init_name_symbol;
    delete length_name_symbol;
    delete null_name_symbol;
    delete Object_name_symbol;
    delete package_info_name_symbol;
    delete question_name_symbol;
    delete serialPersistentFields_name_symbol;
    delete serialVersionUID_name_symbol;
    delete this_name_symbol;
    delete val_name_symbol;*/

    /*delete ConstantValue_literal;
    delete Exceptions_literal;
    delete InnerClasses_literal;
    delete Synthetic_literal;
    delete Deprecated_literal;
    delete LineNumberTable_literal;
    delete LocalVariableTable_literal;
    delete Code_literal;
    delete SourceFile_literal;
    delete EnclosingMethod_literal;

    delete byte_type;
    delete short_type;
    delete int_type;
    delete long_type;
    delete char_type;
    delete float_type;
    delete double_type;
    delete boolean_type;
    delete void_type;
    delete null_type;
    delete no_type;*/

    /*delete annotation_package;
    delete io_package;
    delete lang_package;
    delete util_package;
    delete unnamed_package;*/

    delete cs_table;
    delete ms_table;
    delete mb_table;
    delete gen_table;
    delete assoc_table;
    delete w_table;
    delete r_table;
    delete d_table;
    delete ast_pool;

    /*for(i = 0; i < classpath.Length(); i++) {
        delete classpath[i]->name_symbol;
        delete classpath[i];
    }*/

#ifdef JIKES_DEBUG
    if (option.debug_dump_lex || option.debug_dump_ast ||
        option.debug_unparse_ast)
    {
        Coutput << line_count << " source lines read" << endl
                << class_files_read << " \".class\" files read" << endl
                << class_files_written << " \".class\" files written" << endl
                << input_files_processed << " \".java\" files processed"
                << endl;
    }
#endif // JIKES_DEBUG
}


PackageSymbol* Control::ProcessPackage(const wchar_t* name)
{
    int name_length = wcslen(name);
    wchar_t* package_name = new wchar_t[name_length];
    int length;
    for (length = 0;
         length < name_length && name[length] != U_SLASH; length++)
    {
         package_name[length] = name[length];
    }
    NameSymbol* name_symbol = FindOrInsertName(package_name, length);

    PackageSymbol* package_symbol =
        external_table.FindPackageSymbol(name_symbol);
    if (! package_symbol)
    {
        package_symbol = external_table.InsertPackageSymbol(name_symbol, NULL);
        FindPathsToDirectory(package_symbol);
    }

    while (length < name_length)
    {
        int start = ++length;
        for (int i = 0;
             length < name_length && name[length] != U_SLASH;
             i++, length++)
        {
             package_name[i] = name[length];
        }
        name_symbol = FindOrInsertName(package_name, length - start);
        PackageSymbol* subpackage_symbol =
            package_symbol -> FindPackageSymbol(name_symbol);
        if (! subpackage_symbol)
        {
            subpackage_symbol =
                package_symbol -> InsertPackageSymbol(name_symbol);
            FindPathsToDirectory(subpackage_symbol);
        }
        package_symbol = subpackage_symbol;
    }

    delete [] package_name;
    return package_symbol;
}


//
// When searching for a subdirectory in a zipped file, it must already be
// present in the hierarchy.
//
DirectorySymbol* Control::FindSubdirectory(PathSymbol* path_symbol,
                                           wchar_t* name, int name_length)
{
    wchar_t* directory_name = new wchar_t[name_length + 1];

    DirectorySymbol* directory_symbol = path_symbol -> RootDirectory();
    for (int start = 0, end;
         directory_symbol && start < name_length;
         start = end + 1)
    {
        end = start;
        for (int i = 0; end < name_length && name[end] != U_SLASH; i++, end++)
             directory_name[i] = name[end];
        NameSymbol* name_symbol = FindOrInsertName(directory_name,
                                                   end - start);
        directory_symbol =
            directory_symbol -> FindDirectorySymbol(name_symbol);
    }

    delete [] directory_name;
    return directory_symbol;
}


//
// When searching for a directory in the system, if it is not already present
// in the hierarchy insert it and attempt to read it from the system...
//
#ifdef UNIX_FILE_SYSTEM
DirectorySymbol* Control::ProcessSubdirectories(wchar_t* source_name,
                                                int source_name_length,
                                                bool source_dir)
{
    int name_length = (source_name_length < 0 ? 0 : source_name_length);
    char* input_name = new char[name_length + 1];
    for (int i = 0; i < name_length; i++)
        input_name[i] = source_name[i];
    input_name[name_length] = U_NULL;

    DirectorySymbol* directory_symbol = NULL;
    struct stat status;
    if (SystemStat(input_name, &status) == 0 &&
        (status.st_mode & JIKES_STAT_S_IFDIR))
    {
        directory_symbol = system_table ->
            FindDirectorySymbol(status.st_dev, status.st_ino);
    }

    if (! directory_symbol)
    {
        if (input_name[0] == U_SLASH) // file name starts with '/'
        {
            directory_symbol =
                new DirectorySymbol(FindOrInsertName(source_name, name_length),
                                    classpath[dot_classpath_index],
                                    source_dir);
            directory_symbol -> ReadDirectory();
            system_directories.Next() = directory_symbol;
            system_table -> InsertDirectorySymbol(status.st_dev,
                                                  status.st_ino,
                                                  directory_symbol);
        }
        else
        {
            wchar_t* name = new wchar_t[name_length + 1];
            for (int i = 0; i < name_length; i++)
                name[i] = source_name[i];
            name[name_length] = U_NULL;

            // Start at the dot directory.
            directory_symbol =
                classpath[dot_classpath_index] -> RootDirectory();

            wchar_t* directory_name = new wchar_t[name_length];
            int end = 0;
            for (int start = end; start < name_length; start = end)
            {
                int length;
                for (length = 0;
                     end < name_length && name[end] != U_SLASH;
                     length++, end++)
                {
                    directory_name[length] = name[end];
                }

                if (length != 1 || directory_name[0] != U_DOT)
                {
                    // Not the current directory.
                    if (length == 2 && directory_name[0] == U_DOT &&
                        directory_name[1] == U_DOT)
                    {
                        // keep the current directory
                        if (directory_symbol -> Identity() == dot_name_symbol ||
                            directory_symbol -> Identity() == dot_dot_name_symbol)
                        {
                            DirectorySymbol* subdirectory_symbol =
                                directory_symbol -> FindDirectorySymbol(dot_dot_name_symbol);
                            if (! subdirectory_symbol)
                                subdirectory_symbol =
                                    directory_symbol -> InsertDirectorySymbol(dot_dot_name_symbol,
                                                                              source_dir);
                            directory_symbol = subdirectory_symbol;
                        }
                        else directory_symbol = directory_symbol -> owner -> DirectoryCast();
                    }
                    else
                    {
                        NameSymbol* name_symbol =
                            FindOrInsertName(directory_name, length);
                        DirectorySymbol* subdirectory_symbol =
                            directory_symbol -> FindDirectorySymbol(name_symbol);
                        if (! subdirectory_symbol)
                            subdirectory_symbol =
                                directory_symbol -> InsertDirectorySymbol(name_symbol,
                                                                          source_dir);
                        directory_symbol = subdirectory_symbol;
                    }
                }

                for (end++;
                     end < name_length && name[end] == U_SLASH;
                     end++); // skip all extra '/'
            }

            //
            // Insert the new directory into the system table to avoid
            // duplicates, in case the same directory is specified with
            // a different name.
            //
            if (directory_symbol !=
                classpath[dot_classpath_index] -> RootDirectory())
            {
                // Not the dot directory.
                system_table -> InsertDirectorySymbol(status.st_dev,
                                                      status.st_ino,
                                                      directory_symbol);
                directory_symbol -> ReadDirectory();
            }

            delete [] directory_name;
            delete [] name;
        }
    }

    delete [] input_name;
    return directory_symbol;
}
#elif defined(WIN32_FILE_SYSTEM)
DirectorySymbol* Control::ProcessSubdirectories(wchar_t* source_name,
                                                int source_name_length,
                                                bool source_dir)
{
    DirectorySymbol* directory_symbol =
        classpath[dot_classpath_index] -> RootDirectory();

    int name_length = (source_name_length < 0 ? 0 : source_name_length);
    wchar_t* name = new wchar_t[name_length + 1];
    char* input_name = new char[name_length + 1];
    for (int i = 0; i < name_length; i++)
        input_name[i] = name[i] = source_name[i];
    input_name[name_length] = name[name_length] = U_NULL;

    if (name_length >= 2 && Case::IsAsciiAlpha(input_name[0]) &&
        input_name[1] == U_COLON) // a disk was specified
    {
        char disk = input_name[0];
        option.SaveCurrentDirectoryOnDisk(disk);
        if (SetCurrentDirectory(input_name))
        {
            // First, get the right size.
            DWORD directory_length = GetCurrentDirectory(0, input_name);
            char* full_directory_name = new char[directory_length + 1];
            DWORD length = GetCurrentDirectory(directory_length, full_directory_name);
            if (length <= directory_length)
            {
                // Turn '\' to '/'.
                for (char* ptr = full_directory_name; *ptr; ptr++)
                    *ptr = (*ptr != U_BACKSLASH ? *ptr : (char) U_SLASH);

                char* current_directory = option.GetMainCurrentDirectory();
                int prefix_length = strlen(current_directory);
                int start = (prefix_length <= (int) length &&
                             Case::StringSegmentEqual(current_directory,
                                                      full_directory_name,
                                                      prefix_length) &&
                             (full_directory_name[prefix_length] == U_SLASH ||
                              full_directory_name[prefix_length] == U_NULL)
                             ? prefix_length + 1
                             : 0);

                if (start > (int) length)
                    name_length = 0;
                else if (start <= (int) length) // note that we can assert that (start != length)
                {
                    delete [] name;
                    name_length = length - start;
                    name = new wchar_t[name_length + 1];
                    for (int k = 0, i = start; i < (int) length; i++, k++)
                        name[k] = full_directory_name[i];
                    name[name_length] = U_NULL;
                }
            }

            delete [] full_directory_name;
        }

        // Reset the current directory on this disk.
        option.ResetCurrentDirectoryOnDisk(disk);
        option.SetMainCurrentDirectory(); // Reset the real current directory.
    }

    int end;
    if (name_length > 2 && Case::IsAsciiAlpha(name[0]) &&
        name[1] == U_COLON && name[2] == U_SLASH)
    {
        end = 3;
    }
    else
    {
        for (end = 0;
             end < name_length && name[end] == U_SLASH;
             end++); // keep all extra leading '/'
    }

    wchar_t* directory_name = new wchar_t[name_length];
    int length;
    if (end > 0)
    {
        for (length = 0; length < end; length++)
            directory_name[length] = name[length];
        NameSymbol* name_symbol = FindOrInsertName(directory_name, length);
        DirectorySymbol* subdirectory_symbol =
            directory_symbol -> FindDirectorySymbol(name_symbol);
        if (! subdirectory_symbol)
            subdirectory_symbol =
                directory_symbol -> InsertDirectorySymbol(name_symbol,
                                                          source_dir);
        directory_symbol = subdirectory_symbol;
    }

    for (int start = end; start < name_length; start = end)
    {
        for (length = 0;
             end < name_length && name[end] != U_SLASH;
             length++, end++)
        {
            directory_name[length] = name[end];
        }

        if (length != 1 || directory_name[0] != U_DOT)
        {
            // Not the current directory.
            if (length == 2 && directory_name[0] == U_DOT &&
                directory_name[1] == U_DOT)
            {
                // Keep the current directory.
                if (directory_symbol -> Identity() == dot_name_symbol ||
                    directory_symbol -> Identity() == dot_dot_name_symbol)
                {
                    DirectorySymbol* subdirectory_symbol =
                        directory_symbol -> FindDirectorySymbol(dot_dot_name_symbol);
                    if (! subdirectory_symbol)
                        subdirectory_symbol =
                            directory_symbol -> InsertDirectorySymbol(dot_dot_name_symbol,
                                                                      source_dir);
                    directory_symbol = subdirectory_symbol;
                }
                else directory_symbol = directory_symbol -> owner -> DirectoryCast();
            }
            else
            {
                NameSymbol* name_symbol = FindOrInsertName(directory_name,
                                                           length);
                DirectorySymbol* subdirectory_symbol =
                    directory_symbol -> FindDirectorySymbol(name_symbol);
                if (! subdirectory_symbol)
                    subdirectory_symbol =
                        directory_symbol -> InsertDirectorySymbol(name_symbol,
                                                                  source_dir);
                directory_symbol = subdirectory_symbol;
            }
        }

        for (end++;
             end < name_length && name[end] == U_SLASH;
             end++); // skip all extra '/'
    }

    directory_symbol -> ReadDirectory();

    delete [] directory_name;
    delete [] name;
    delete [] input_name;
    return directory_symbol;
}
#endif // WIN32_FILE_SYSTEM


void Control::ProcessNewInputFiles(SymbolSet& file_set, char** arguments)
{
    unsigned i;
    for (i = 0; i < bad_input_filenames.Length(); i++)
        delete [] bad_input_filenames[i];
    bad_input_filenames.Reset();
    for (i = 0; i < unreadable_input_filenames.Length(); i++)
        delete [] unreadable_input_filenames[i];
    unreadable_input_filenames.Reset();

    //
    // Process all file names specified in command line. By this point, only
    // filenames should remain in arguments - constructing the Option should
    // have filtered out all options and expanded @files.
    //
    if (arguments)
    {
        int j = 0;
        while (arguments[j])
        {
            char* file_name = arguments[j++];
            unsigned file_name_length = strlen(file_name);

            wchar_t* name = new wchar_t[file_name_length + 1];
            for (unsigned i = 0; i < file_name_length; i++)
                name[i] = (file_name[i] != U_BACKSLASH ? file_name[i]
                           : (wchar_t) U_SLASH); // Change '\' to '/'.
            name[file_name_length] = U_NULL;

            //
            // File must be of the form xxx.java where xxx is a
            // character string consisting of at least one character.
            //
            if (file_name_length < FileSymbol::java_suffix_length ||
                (! FileSymbol::IsJavaSuffix(&file_name[file_name_length - FileSymbol::java_suffix_length])))
            {
                bad_input_filenames.Next() = name;
            }
            else
            {
                FileSymbol* file_symbol =
                    FindOrInsertJavaInputFile(name,
                                              file_name_length - FileSymbol::java_suffix_length);

                if (! file_symbol)
                    unreadable_input_filenames.Next() = name;
                else
                {
                    delete [] name;
                    file_set.AddElement(file_symbol);
                }
            }
        }
    }
}


FileSymbol* Control::FindOrInsertJavaInputFile(DirectorySymbol* directory_symbol,
                                               NameSymbol* file_name_symbol)
{
    FileSymbol* file_symbol = NULL;

    int length = file_name_symbol -> Utf8NameLength() +
        FileSymbol::java_suffix_length;
    char* java_name = new char[length + 1]; // +1 for \0
    strcpy(java_name, file_name_symbol -> Utf8Name());
    strcat(java_name, FileSymbol::java_suffix);

    DirectoryEntry* entry = directory_symbol -> FindEntry(java_name, length);
    if (entry)
    {
        file_symbol = directory_symbol -> FindFileSymbol(file_name_symbol);

        if (! file_symbol)
        {
            file_symbol =
                directory_symbol -> InsertFileSymbol(file_name_symbol);
            file_symbol -> directory_symbol = directory_symbol;
            file_symbol -> SetJava();
        }

        file_symbol -> mtime = entry -> Mtime();
    }

    delete [] java_name;
    return file_symbol;
}


FileSymbol* Control::FindOrInsertJavaInputFile(wchar_t* name, int name_length)
{
    FileSymbol* file_symbol = NULL;

    //
    // The name has been preprocessed so that if it contains any
    // slashes it is a forward slash. In the loop below we look
    // for the occurrence of the first slash (if any) that separates
    // the file name from its directory name.
    //
    DirectorySymbol* directory_symbol;
    NameSymbol* file_name_symbol;
#ifdef UNIX_FILE_SYSTEM
    int len;
    for (len = name_length - 1; len >= 0 && name[len] != U_SLASH; len--)
        ;
    directory_symbol = ProcessSubdirectories(name, len, true);
    file_name_symbol = FindOrInsertName(&name[len + 1],
                                        name_length - (len + 1));
#elif defined(WIN32_FILE_SYSTEM)
    int len;
    for (len = name_length - 1;
         len >= 0 && name[len] != U_SLASH && name[len] != U_COLON;
         len--);

    directory_symbol = ProcessSubdirectories(name,
                                             (name[len] == U_COLON ? len + 1
                                              : len),
                                             true);
    file_name_symbol = FindOrInsertName(&name[len + 1],
                                        name_length - (len + 1));
#endif // WIN32_FILE_SYSTEM

    for (unsigned i = 1; i < classpath.Length(); i++)
    {
        if (i == dot_classpath_index) // the current directory (.).
        {
            file_symbol = FindOrInsertJavaInputFile(directory_symbol,
                                                    file_name_symbol);
            if (file_symbol)
                break;
        }
        else if (classpath[i] -> IsZip())
        {
            DirectorySymbol* directory_symbol = FindSubdirectory(classpath[i],
                                                                 name, len);
            if (directory_symbol)
            {
                file_symbol =
                    directory_symbol -> FindFileSymbol(file_name_symbol);
                if (file_symbol && file_symbol -> IsJava())
                     break;
                else file_symbol = NULL;
            }
        }
    }

    //
    // If the file was found, return it; otherwise, in case the (.) directory
    // was not specified in the classpath, search for the file in it...
    //
    return file_symbol ? file_symbol
        : FindOrInsertJavaInputFile(directory_symbol, file_name_symbol);
}


PackageSymbol* Control::FindOrInsertPackage(LexStream* lex_stream,
                                            AstName* name)
{
    PackageSymbol* package;

    if (name -> base_opt)
    {
        package = FindOrInsertPackage(lex_stream, name -> base_opt);
        NameSymbol* name_symbol =
            lex_stream -> NameSymbol(name -> identifier_token);
        PackageSymbol* subpackage = package -> FindPackageSymbol(name_symbol);
        if (! subpackage)
            subpackage = package -> InsertPackageSymbol(name_symbol);
        package = subpackage;
    }
    else
    {
        NameSymbol* name_symbol =
            lex_stream -> NameSymbol(name -> identifier_token);
        package = external_table.FindPackageSymbol(name_symbol);
        if (! package)
            package = external_table.InsertPackageSymbol(name_symbol, NULL);
    }

    FindPathsToDirectory(package);
    return package;
}


void Control::ProcessFile(FileSymbol* file_symbol, StoragePool* ast_pool)
{
    ProcessHeaders(file_symbol);

    //
    // As long as there are new bodies, ...
    //
    for (unsigned i = 0; i < needs_body_work.Length(); i++)
    {
        assert(semantic.Length() == 0);

        //
        // These bodies are not necessarily in file_symbol; they
        // might be in another FileSymbol used by file_symbol.
        //
        ProcessBodies(needs_body_work[i], ast_pool);
    }
    needs_body_work.Reset();
}


void Control::ProcessHeaders(FileSymbol* file_symbol)
{
    if (file_symbol -> semantic)
        return;
    input_java_file_set.AddElement(file_symbol);

    bool initial_invocation = (semantic.Length() == 0);

    if (option.verbose)
    {
        Coutput << "[read "
                << file_symbol -> FileName()
                << "]" << endl;
    }

    if (! file_symbol -> lex_stream)
         scanner -> Scan(file_symbol);
    else file_symbol -> lex_stream -> Reset();

    if (file_symbol -> lex_stream) // do we have a successful scan!
    {
        if (! file_symbol -> compilation_unit)
            file_symbol -> compilation_unit =
                parser -> HeaderParse(file_symbol -> lex_stream);
        //
        // If we have a compilation unit, analyze it, process its types.
        //
        if (file_symbol -> compilation_unit)
        {
            assert(! file_symbol -> semantic);

            if (! file_symbol -> package)
                ProcessPackageDeclaration(file_symbol,
                                          file_symbol -> compilation_unit -> package_declaration_opt);
            file_symbol -> semantic = new Semantic(*this, file_symbol);
            semantic.Next() = file_symbol -> semantic;
            file_symbol -> semantic -> ProcessTypeNames();
        }
    }

    if (initial_invocation)
        ProcessMembers();
}


void Control::ProcessMembers()
{
    Tuple<TypeSymbol*> partially_ordered_types(1024);
    SymbolSet needs_member_work(101);
    TypeCycleChecker cycle_checker(partially_ordered_types);
    TopologicalSort topological_sorter(needs_member_work,
                                       partially_ordered_types);

    unsigned start = 0;
    while (start < semantic.Length())
    {
        needs_member_work.SetEmpty();

        do
        {
            //
            // Check whether or not there are cycles in this new batch of
            // types. Create a partial order of the types (cycles are ordered
            // arbitrarily) and place the result in partially_ordered_types.
            //
            cycle_checker.PartialOrder(semantic, start);
            start = semantic.Length(); // next starting point

            //
            // Process the extends and implements clauses.
            //
            for (unsigned j = 0; j < partially_ordered_types.Length(); j++)
            {
                TypeSymbol* type = partially_ordered_types[j];
                needs_member_work.AddElement(type);
                type -> ProcessTypeHeaders();
                type -> semantic_environment -> sem ->
                    types_to_be_processed.AddElement(type);
            }
        } while (start < semantic.Length());

        //
        // Partially order the collection of types in needs_member_work and
        // place the result in partially_ordered_types. This reordering is
        // based on the complete "supertype" information computed in
        // ProcessTypeHeaders.
        //
        topological_sorter.Sort();
        for (unsigned i = 0; i < partially_ordered_types.Length(); i++)
        {
            TypeSymbol* type = partially_ordered_types[i];
            needs_body_work.Next() = type;
            type -> ProcessMembers();
        }
    }

    needs_member_work.SetEmpty();

    semantic.Reset();
}


void Control::CollectTypes(TypeSymbol* type, Tuple<TypeSymbol*>& types)
{
    types.Next() = type;

    for (unsigned j = 0; j < type -> NumAnonymousTypes(); j++)
        CollectTypes(type -> AnonymousType(j), types);

    if (type -> local)
    {
        for (TypeSymbol* local_type = (TypeSymbol*) type -> local -> FirstElement();
             local_type;
             local_type = (TypeSymbol*) type -> local -> NextElement())
        {
            CollectTypes(local_type, types);
        }
    }

    if (type -> non_local)
    {
        for (TypeSymbol* non_local_type = (TypeSymbol*) type -> non_local -> FirstElement();
             non_local_type;
             non_local_type = (TypeSymbol*) type -> non_local -> NextElement())
        {
            CollectTypes(non_local_type, types);
        }
    }
}


void Control::ProcessBodies(TypeSymbol *type, StoragePool *ast_pool) {
    Semantic *sem = type->semantic_environment->sem;

    if (type->declaration &&
        !sem->compilation_unit->BadCompilationUnitCast()) {
#ifdef WIN32_FILE_SYSTEM
        if (! type -> file_symbol -> IsZip())
        {
            int length = type -> Utf8NameLength() +
                FileSymbol::class_suffix_length;
            char* classfile_name = new char[length + 1]; // +1 for "\0"
            strcpy(classfile_name, type -> Utf8Name());
            strcat(classfile_name, FileSymbol::class_suffix);

            DirectorySymbol* directory =
                type -> file_symbol -> OutputDirectory();
            DirectoryEntry* entry =
                directory -> FindCaseInsensitiveEntry(classfile_name, length);

            //
            // If an entry is found and it is not identical (in a
            // case-sensitive test) to the name of the type, issue an
            // appropriate message.
            //
            if (entry && strcmp(classfile_name, entry -> name) != 0)
            {
                wchar_t* entry_name = new wchar_t[entry -> length + 1];
                for (int i = 0; i < length; i++)
                    entry_name[i] = entry -> name[i];
                entry_name[entry -> length] = U_NULL;
                sem -> ReportSemError(SemanticError::FILE_FILE_CONFLICT,
                                      type -> declaration -> identifier_token,
                                      type -> Name(), entry_name,
                                      directory -> Name());
                delete [] entry_name;
            }
            delete [] classfile_name;
        }
#endif // WIN32_FILE_SYSTEM

        if (!parser->InitializerParse(sem->lex_stream,
                                      type->declaration)) {
            // Mark that syntax errors were detected.
            sem->compilation_unit->MarkBad();
        } else {
            type->CompleteSymbolTable();
            if (!parser->BodyParse(sem->lex_stream, type->declaration)) {
                // Mark that syntax errors were detected.
                sem->compilation_unit->MarkBad();
            } else type->ProcessExecutableBodies();
        }

        if (sem->NumErrors() == 0 &&
            sem->lex_stream->NumBadTokens() == 0 &&
            !sem->compilation_unit->BadCompilationUnitCast()) {
            auto *types = new Tuple<TypeSymbol *>(1024);
            CollectTypes(type, *types);

            //
            // Get Structural info for pattern detection.
            //
            for (unsigned k = 0; k < types->Length(); k++) {
                TypeSymbol *type = (*types)[k];
                ExtractStructure(w_table, r_table, d_table, cs_table, mb_table, ms_table, gen_table, assoc_table, type,
                                 ast_pool);
            }

            //
            // If we are supposed to generate code, do so now !!!
            //
            if (option.bytecode) {
                for (unsigned k = 0; k < types->Length(); k++) {
                    TypeSymbol *type = (*types)[k];
                    // Make sure the literal is available for bytecode.
                    type->file_symbol->SetFileNameLiteral(this);
                    ByteCode *code = new ByteCode(type);
                    code->GenerateCode();
                    delete code;
                }
            }

            //
            // If no error was detected while generating code, then
            // start cleaning up.
            //
            if (!option.nocleanup) {
                if (sem->NumErrors() == 0) {
                    for (unsigned k = 0; k < types->Length(); k++) {
                        TypeSymbol *type = (*types)[k];
                        delete type->semantic_environment;
                        type->semantic_environment = NULL;
                        type->declaration->semantic_environment = NULL;
                    }
                }
                delete types;
            }
        } else {
            //Coutput << "failed " << type->fully_qualified_name->value << endl;
            //Coutput << "NumErrors: " << sem->NumErrors() << endl;
            //Coutput << "NumBadTokens: " << sem->lex_stream->NumBadTokens() << endl;
            //sem->PrintMessages();
        }
    }

    sem->types_to_be_processed.RemoveElement(type);

    if (sem->types_to_be_processed.Size() == 0) {
        // All types belonging to this compilation unit have been processed.
        CheckForUnusedImports(sem);
        if (!option.nocleanup) {
            //CleanUp(sem->source_file_symbol);
        }
    }
}

void Control::CheckForUnusedImports(Semantic* sem)
{
    if (sem -> NumErrors() != 0 ||
        sem -> lex_stream -> NumBadTokens() != 0 ||
        sem -> compilation_unit -> BadCompilationUnitCast())
    {
        //
        // It's not worth checking for unused imports if compilation
        // wasn't successful; we may well have just not got round to
        // compiling the relevant code, and if there were errors, the
        // user has more important things to worry about than unused
        // imports!
        //
        return;
    }

    for (unsigned i = 0;
         i < sem -> compilation_unit -> NumImportDeclarations(); ++i)
    {
        AstImportDeclaration* import_declaration =
            sem -> compilation_unit -> ImportDeclaration(i);
        Symbol* symbol = import_declaration -> name -> symbol;
        if (import_declaration -> star_token_opt)
        {
            PackageSymbol* package = symbol -> PackageCast();
            if (package &&
                ! sem -> referenced_package_imports.IsElement(package))
            {
                sem -> ReportSemError(SemanticError::UNUSED_PACKAGE_IMPORT,
                                      import_declaration,
                                      package -> PackageName());
            }
        }
        else
        {
            TypeSymbol* import_type = symbol -> TypeCast();
            if (import_type &&
                ! sem -> referenced_type_imports.IsElement(import_type))
            {
                sem -> ReportSemError(SemanticError::UNUSED_TYPE_IMPORT,
                                      import_declaration,
                                      import_type -> ContainingPackage() -> PackageName(),
                                      import_type -> ExternalName());
            }
        }
    }
}

//
// Introduce the main package and the current package.
// This procedure is invoked directly only while doing
// an incremental compilation.
//
void Control::ProcessPackageDeclaration(FileSymbol* file_symbol,
                                        AstPackageDeclaration* package_declaration)
{
    file_symbol -> package = (package_declaration
                              ? FindOrInsertPackage(file_symbol -> lex_stream,
                                                    package_declaration -> name)
                              : unnamed_package);

    for (unsigned i = 0; i < file_symbol -> lex_stream -> NumTypes(); i++)
    {
        TokenIndex identifier_token = file_symbol -> lex_stream ->
            Next(file_symbol -> lex_stream -> Type(i));
        if (file_symbol -> lex_stream -> Kind(identifier_token) ==
            TK_Identifier)
        {
            NameSymbol* name_symbol =
                file_symbol -> lex_stream -> NameSymbol(identifier_token);
            if (! file_symbol -> package -> FindTypeSymbol(name_symbol))
            {
                TypeSymbol* type = file_symbol -> package ->
                    InsertOuterTypeSymbol(name_symbol);
                type -> file_symbol = file_symbol;
                type -> outermost_type = type;
                type -> supertypes_closure = new SymbolSet;
                type -> subtypes = new SymbolSet;
                type -> SetOwner(file_symbol -> package);
                type -> SetSignature(*this);
                type -> MarkSourcePending();

                //
                // If this type is contained in the unnamed package add it to
                // the set unnamed_package_types if a type of similar name was
                // not already there.
                //
                if (! package_declaration &&
                    unnamed_package_types.Image(type -> Identity()) == NULL)
                {
                    unnamed_package_types.AddElement(type);
                }
            }
        }
    }
}


void Control::CleanUp(FileSymbol* file_symbol)
{
    Semantic* sem = file_symbol -> semantic;

    if (sem)
    {
#ifdef JIKES_DEBUG
        if (option.debug_dump_lex)
        {
            sem -> lex_stream -> Reset(); // rewind input and ...
            sem -> lex_stream -> Dump();  // dump it!
        }
        if (option.debug_dump_ast)
            sem -> compilation_unit -> Print(*sem -> lex_stream);
        if (option.debug_unparse_ast)
        {
            if (option.debug_unparse_ast_debug)
              {
                // which of these is correct?
                sem -> compilation_unit -> debug_unparse = true;
                Ast::debug_unparse = true;
              }
            sem -> compilation_unit -> Unparse(sem -> lex_stream,
                                               "unparsed/");
        }
#endif // JIKES_DEBUG
        //sem -> PrintMessages();
        if (sem -> return_code > 0)
            return_code = 1;

        file_symbol -> CleanUp();
    }
}

 
#ifdef HAVE_JIKES_NAMESPACE
} // Close namespace Jikes block
#endif

