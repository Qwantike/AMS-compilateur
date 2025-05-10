//  A compiler from a very simple Pascal-like structured language LL(k)
//  to 64-bit 80x86 Assembly langage
//  Copyright (C) 2019 Pierre Jourlin
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.

// Build with "make compilateur"

#include <string>
#include <iostream>
#include <cstdlib>
#include <set>

using namespace std;

// VARIABLES GLOBALES
char current;		   // Current car
char nextcar;		   // Next character
bool firstRead = true; // First read of the next character
set<char> declaredVariables;

void ReadChar(void)
{ // Read character and skip spaces until
	// non space character is read
	if (firstRead)
	{
		while (cin.get(nextcar) && (nextcar == ' ' || nextcar == '\t' || nextcar == '\n'))
			cin.get(nextcar);
		firstRead = false;
	}
	else
	{
		current = nextcar;
		while (cin.get(nextcar) && (nextcar == ' ' || nextcar == '\t' || nextcar == '\n'))
			cin.get(nextcar);
	}
}

void Error(string s)
{
	cerr << s << endl;
	exit(-1);
}

// ArithmeticExpression := Term {AdditiveOperator Term}
// Term := Digit | "(" ArithmeticExpression ")"
// AdditiveOperator := "+" | "-"
// Digit := "0"|"1"|"2"|"3"|"4"|"5"|"6"|"7"|"8"|"9"

void AdditiveOperator(void)
{
	if (current == '+' || current == '-')
		ReadChar();
	else
		Error("Opérateur additif attendu"); // Additive operator expected
}

/* void Digit(void)
{
	if ((current < '0') || (current > '9'))
		Error("Chiffre attendu"); // Digit expected
	else
	{
		cout << "\tpush $" << current << endl;
		ReadChar();
	}
}*/

void Number(void)
{
	long val = 0;
	if (current < '0' || current > '9')
		Error("Nombre attendu");

	while (current >= '0' && current <= '9')
	{
		// current - '0' change le caractère en sa valeur entière (-48)
		val = val * 10 + (current - '0');
		ReadChar();
	}
	cout << "\tpush $" << val << endl;
}

void ArithmeticExpression(void); // Called by Term() and calls Term()

void Term(void)
{
	if (current == '(')
	{
		ReadChar();
		ArithmeticExpression();
		if (current != ')')
			Error("')' était attendu"); // ")" expected
		else
			ReadChar();
	}
	else if (current >= '0' && current <= '9')
		Number();
	else
		Error("'(' ou chiffre attendu");
}

void ArithmeticExpression(void)
{
	char adop;
	Term();
	while (current == '+' || current == '-')
	{
		adop = current; // Save operator in local variable
		AdditiveOperator();
		Term();
		cout << "\tpop %rbx" << endl; // get first operand
		cout << "\tpop %rax" << endl; // get second operand
		if (adop == '+')
			cout << "\taddq	%rbx, %rax" << endl; // add both operands
		else
			cout << "\tsubq	%rbx, %rax" << endl; // substract both operands
		cout << "\tpush %rax" << endl;			 // store result
	}
}

void DeclarationPart()
{
	if (current != '[')
		return; // Pas de partie déclaration

	ReadChar(); // Consomme '['

	if (current < 'a' || current > 'z')
		Error("Lettre attendue dans la déclaration");

	// Premier identifiant
	declaredVariables.insert(current);
	cout << current << ":\t.quad 0" << endl;
	ReadChar();

	while (current == ',')
	{
		ReadChar(); // consomme la virgule

		if (current < 'a' || current > 'z')
			Error("Lettre attendue après ','");

		if (declaredVariables.count(current))
			Error("Variable déclarée en double");

		declaredVariables.insert(current);
		cout << current << ":\t.quad 0" << endl;
		ReadChar();
	}

	if (current != ']')
		Error("']' attendu à la fin de la déclaration");

	ReadChar(); // consomme ']'
}

// Modifié : RelationalOperator vérifie l'opérateur relationnel et la suite de l'expression
string RelationalOperator(void)
{
	string op;

	// Vérification des opérateurs relationnels à deux caractères
	if (current == '=' && nextcar == '=')
	{
		op = "==";
		ReadChar(); // Consomme '='
		ReadChar(); // Lookahead
	}
	else if (current == '!' && nextcar == '=')
	{
		op = "!=";
		ReadChar(); // Consomme '!'
		ReadChar(); // Lookahead
	}
	else if (current == '<' && nextcar == '>')
	{
		op = "<>";
		ReadChar(); // Consomme '>'
		ReadChar(); // Lookahead
	}
	else if (current == '<' && nextcar == '=')
	{
		op = "<=";
		ReadChar(); // Consomme '='
		ReadChar(); // Lookahead
	}
	else if (current == '>' && nextcar == '=')
	{
		op = ">=";
		ReadChar(); // Consomme '='
		ReadChar(); // Lookahead
	}
	else if (current == '=' || current == '<' || current == '>')
	{
		op = string(1, current);
		ReadChar();
	}
	else
	{
		Error("Opérateur relationnel attendu");
	}

	// Ignore whitespace after operator
	while (isspace(current) && !cin.eof())
	{
		ReadChar();
	}

	return op;
}

void AssignmentStatement()
{
	char varName = current;

	// Vérifier que la variable à gauche est déclarée
	if (declaredVariables.find(varName) == declaredVariables.end())
		Error("Variable à gauche non déclarée");

	ReadChar(); // Consommer le '='

	Expression(); // Lire l'expression à droite

	// Générer le code pour l'affectation
	cout << "\tpop " << varName << endl;
}

void StatementPart()
{
	// On appelle AssignmentStatement pour chaque instruction
	while (current != '.' && current != EOF)
	{
		AssignmentStatement();
		if (current == ';')
			ReadChar(); // Consomme le ';'
		else
			Error("Point-virgule attendu");
	}
}

void SimpleExpression(void)
{
	Term();																									  // Première partie de l'expression
	while (current == '+' || current == '-' || current == '|' /* '|' pour le OU logique */ || current == '&') // Ajout du ET logique
	{
		char oper = current;		  // On enregistre l'opérateur
		AdditiveOperator();			  // Consomme +, -, etc.
		Term();						  // Traite la seconde partie
		cout << "\tpop %rbx" << endl; // Prend le deuxième opérande
		cout << "\tpop %rax" << endl; // Prend le premier opérande
		if (oper == '+')
			cout << "\taddq %rbx, %rax" << endl; // Addition
		else if (oper == '-')
			cout << "\tsubq %rbx, %rax" << endl; // Soustraction
		else if (oper == '|')
			cout << "\torq %rbx, %rax" << endl;	 // OU logique
		else if (oper == '&')					 // ET logique
			cout << "\tandq %rbx, %rax" << endl; // ET logique
		cout << "\tpush %rax" << endl;			 // Résultat
	}
}

void Expression(void)
{
	SimpleExpression(); // Lit la première partie de l'expression

	// Si un opérateur relationnel est trouvé, on délègue à RelationalOperator
	if (current == '=' || current == '<' || current == '>' || current == '|' || current == '&')
	{
		string op = RelationalOperator(); // Lit l'opérateur relationnel
		SimpleExpression();				  // Lire la seconde partie
		cout << "\tpop %rbx" << endl;
		cout << "\tpop %rax" << endl;
		cout << "\tcmp %rbx, %rax" << endl;

		if (op == "==")
			cout << "\tsete %al" << endl;
		else if (op == "!=")
			cout << "\tsetne %al" << endl;
		else if (op == "<")
			cout << "\tsetl %al" << endl;
		else if (op == "<=")
			cout << "\tsetle %al" << endl;
		else if (op == ">")
			cout << "\tsetg %al" << endl;
		else if (op == ">=")
			cout << "\tsetge %al" << endl;
		else if (op == "|")
			cout << "\tsetne %al" << endl; // OU logique (si non égal, alors vrai)

		cout << "\tmovzbq %al, %rax" << endl; // Convertit en 64 bits
		cout << "\tpush %rax" << endl;
	}
	// Gestion des opérateurs logiques && et ||
	else if (current == '&' && nextcar == '&') // Opérateur ET logique &&
	{
		ReadChar();			// Consomme le premier '&'
		ReadChar();			// Consomme le deuxième '&'
		SimpleExpression(); // Lire la deuxième partie de l'expression

		cout << "\tpop %rbx" << endl;
		cout << "\tpop %rax" << endl;
		cout << "\tandq %rbx, %rax" << endl; // ET logique
		cout << "\tpush %rax" << endl;
	}
	else if (current == '|' && nextcar == '|') // Opérateur OU logique ||
	{
		ReadChar();			// Consomme le premier '|'
		ReadChar();			// Consomme le deuxième '|'
		SimpleExpression(); // Lire la deuxième partie de l'expression

		cout << "\tpop %rbx" << endl;
		cout << "\tpop %rax" << endl;
		cout << "\torq %rbx, %rax" << endl; // OU logique
		cout << "\tpush %rax" << endl;
	}
}

void Factor(void)
{
	if (current == '(')
	{
		ReadChar();
		Expression();
		if (current != ')')
		{
			Error("')' était attendu");
		}
		ReadChar(); // Consomme ')'
	}
	else if (current == '!')
	{
		ReadChar(); // Consomme '!'
		Factor();	// Effectue la négation
		cout << "\tpop %rax" << endl;
		cout << "\txorq $0xFFFFFFFFFFFFFFFF, %rax" << endl; // Négation bit à bit
		cout << "\tpush %rax" << endl;
	}
	else if (current >= 'a' && current <= 'z')
	{
		cout << "\tpush " << current << endl; // Empiler la variable
		ReadChar();
	}
	else if (current >= '0' && current <= '9')
	{
		Number(); // Nombre
	}
	else
	{
		Error("Facteur attendu");
	}
}

// Fonction main
int main(void)
{
	cout << "\t\t\t# This code was produced by the CERI Compiler" << endl;

	cin.get(current);
	ReadChar();

	if (current == '[')
	{
		cout << "\t.data" << endl;
		DeclarationPart();
	}

	cout << "\t.text\t\t# The following lines contain the program" << endl;
	cout << "\t.globl main\t# The main function must be visible from outside" << endl;
	cout << "main:\t\t\t# The main function body :" << endl;
	cout << "\tmovq %rsp, %rbp\t# Save the position of the stack's top" << endl;

	StatementPart();

	cout << "\tmovq %rbp, %rsp\t\t# Restore the position of the stack's top" << endl;
	cout << "\tret\t\t\t# Return from main function" << endl;

	if (cin.get(current))
	{
		cerr << "Caractères en trop à la fin du programme : [" << current << "]";
		Error(".");
	}
}
/***BAZAR
 * Vérifivation de la déclaration d'une variable
	if (!declaredVariables.count(var))
		Error("Variable '" + string(1, var) + "' non déclarée");
 *
*/
