//
// Created by Andrew Yang on 8/30/21.
//

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct {
    const wchar_t* start;
    const wchar_t* current;
    int line;
} Scanner;

Scanner scanner;

void initScanner(const wchar_t* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static bool isAtEnd() {
    return *scanner.current == L'\0';
}

static wchar_t advance() {
    scanner.current++;
    return scanner.current[-1];
}

static wchar_t peek() {
    return *scanner.current;
}

static wchar_t peekNext() {
    if (isAtEnd()) return L'\0';
    return scanner.current[1];
}

static bool match(wchar_t expected) {
    if (isAtEnd()) return false;
    if (*scanner.current != expected) return false;
    scanner.current++;
    return true;
}

static bool isAlpha(wchar_t ch) {
    return ((unsigned int)ch >= 0x4E00u && (unsigned int)ch <= 0x2FA1F && !iswpunct(ch)) || iswalpha(ch);
}

static Token makeToken(TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

static Token errorToken(const wchar_t* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)wcslen(message);
    token.line = scanner.line;
    return token;
}

static void skipWhitespace() {
    for (;;) {
        wchar_t c = peek();
        switch (c) {
            case L' ':
            case L'\r':
            case L'\t':
                advance();
                break;
            case L'\n':
                scanner.line++;
                advance();
                break;
            case L'/':
                if (peekNext() == L'/') {
                    // A comment goes until the end of the line.
                    while (peek() != L'\n' && !isAtEnd()) advance();
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static TokenType checkKeyword(int start, int length, const wchar_t* rest, TokenType type) {
    if (scanner.current - scanner.start == start + length
    && memcmp(scanner.start + start, rest, length * sizeof(wchar_t)) == 0) {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

static TokenType identifierType() {
    switch (scanner.start[0]) {
        case L'打': return checkKeyword(1, 1, L"断", TOKEN_BREAK);
        case L'继': return checkKeyword(1, 1, L"续", TOKEN_CONTINUE);
        case L'类': return checkKeyword(1, 0, L"", TOKEN_CLASS);
        case L'切': return checkKeyword(1, 1, L"换", TOKEN_SWITCH);
        case L'案': return checkKeyword(1, 1, L"例", TOKEN_CASE);
        case L'预': return checkKeyword(1, 1, L"设", TOKEN_DEFAULT);
        case L'否': return checkKeyword(1, 1, L"则", TOKEN_ELSE);
        case L'功': return checkKeyword(1, 1, L"能", TOKEN_FUN);
        case L'而': return checkKeyword(1, 0, L"", TOKEN_WHILE);
        case L'对': return checkKeyword(1, 1, L"于", TOKEN_FOR);
        case L'如': return checkKeyword(1, 1, L"果", TOKEN_IF);
        case L'空': return checkKeyword(1, 0, L"", TOKEN_NIL);
        case L'返': return checkKeyword(1, 1, L"回", TOKEN_RETURN);
        case L'超': return checkKeyword(1, 0, L"", TOKEN_SUPER);
        case L'真': return checkKeyword(1, 0, L"", TOKEN_TRUE);
        case L'假': return checkKeyword(1, 0, L"", TOKEN_FALSE);
        case L'这': return checkKeyword(1, 0, L"", TOKEN_THIS);
        case L'变': return checkKeyword(1, 1, L"量", TOKEN_VAR);
        case L'和': return checkKeyword(1, 0, L"", TOKEN_AND);
        case L'或': return checkKeyword(1, 0, L"", TOKEN_OR);
        case L'等': return checkKeyword(1, 0, L"", TOKEN_EQUAL_EQUAL);
        case L'不':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case L'等': return checkKeyword(2, 0, L"", TOKEN_BANG_EQUAL);
                }
            }
            return checkKeyword(1, 0, L"", TOKEN_BANG);
        case L'大':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case L'等':
                        return checkKeyword(2, 0, L"", TOKEN_GREATER_EQUAL);
                }
            }
            return checkKeyword(1, 0, L"", TOKEN_GREATER);
        case L'小':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case L'等': return checkKeyword(2, 0, L"", TOKEN_LESS_EQUAL);
                }
            }
            return checkKeyword(1, 0, L"", TOKEN_LESS);
    }

    return TOKEN_IDENTIFIER;
}

static Token identifier() {
    while (isAlpha(peek()) || iswdigit(peek())) advance();
    return makeToken(identifierType());
}

static Token number() {
    while (iswdigit(peek())) advance();

    // Look for a fractional part.
    if (peek() == L'.' && iswdigit(peekNext())) {
        // Consume the ".".
        advance();

        while (iswdigit(peek())) advance();
    }

    return makeToken(TOKEN_NUMBER);
}

static Token string() {
    while (peek() != L'"' && !isAtEnd()) {
        if (peek() == L'\n') scanner.line++;
        advance();
    }

    if (isAtEnd()) return errorToken(L"Unterminated string.");

    // The closing quote.
    advance();
    return makeToken(TOKEN_STRING);
}

Token scanToken() {
    skipWhitespace();
    scanner.start = scanner.current;

    if (isAtEnd()) return makeToken(TOKEN_EOF);

    wchar_t c = advance();
    if (isAlpha(c)) return identifier();
    if (iswdigit(c)) return number();

    switch (c) {
        case L'（': return makeToken(TOKEN_LEFT_PAREN);
        case L'）': return makeToken(TOKEN_RIGHT_PAREN);
        case L'『':
        case L'「': return makeToken(TOKEN_LEFT_BRACE);
        case L'』':
        case L'」': return makeToken(TOKEN_RIGHT_BRACE);
        case L'；': return makeToken(TOKEN_SEMICOLON);
        case L'，': return makeToken(TOKEN_COMMA);
        case L'。': return makeToken(TOKEN_DOT);
        case L'-':
            return makeToken(match(L'=') ? TOKEN_MINUS_EQUAL
            : match(L'-') ? TOKEN_MINUS_MINUS : TOKEN_MINUS);
        case L'+':
            return makeToken(match(L'=') ? TOKEN_PLUS_EQUAL
            : match(L'+') ? TOKEN_PLUS_PLUS : TOKEN_PLUS);
        case L'/': return makeToken(TOKEN_SLASH);
        case L'*': return makeToken(TOKEN_STAR);
        case L'%': return makeToken(TOKEN_PERCENT);
        case L'：': return makeToken(TOKEN_COLON);
        case L'=': return makeToken(TOKEN_EQUAL);
        case L'"': return string();
        case L'【': return makeToken(TOKEN_LEFT_BRACKET);
        case L'】': return makeToken(TOKEN_RIGHT_BRACKET);
    }

    return errorToken(L"Unexpected character.");
}

Scanner scanner;
