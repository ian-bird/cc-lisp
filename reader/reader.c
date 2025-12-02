#include "reader.h"

Value read(char **input)
{
  switch(**input)
    {
    case '(':
      {
	Values *vs = NULL;
	while(*input != ')')
	  {	    
	    vs = append(vs, read(input));
	  }
	return makeList(vs);
      }

      // now its either a character or a vector
    case '#':
      (*input)++;
      switch(**input)
	{
	  // vector
	case '(':
	  return readVector(input);

	  // character
	case '\\':
	  return readCharacter(input);
	  break;
	  
	default:
	  longjmp(onErr, READ_ERR);
	}
      break;

    case '"':
      return readString(input);

      // comments are ignored and terminated by new line
    case ';':
      while(*input != '\n' && *input != '\r')
	(*input)++;
      return readCharacter(input);

    case '.':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      return readNumber(input);

    case ' ':
    case '\t':
    case '\n':
    case '\r':
      (*input)++;
      return read(input);

    	  
      longjmp(onErr, UNEXPECTED_END);
    }
}
