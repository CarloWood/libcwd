//
// From http://reality.sgi.com/dehnert_engr/cxx/abi.html#mangling-type:
//
// <mangled-name> ::= _Z <encoding>
//
// <encoding> ::= <function name> <bare-function-type>			# Starts with 'C', 'D', 'N', 'S', a digit or a lower case character.
//            ::= <data name>						# idem
//            ::= <special-name>					# Starts with 'T' or 'G'.
//
// <special-name> ::= TV <type>				# virtual table
//                ::= TT <type>				# VTT structure (construction vtable index)
//                ::= TI <type>				# typeinfo structure
//                ::= TS <type>				# typeinfo name (null-terminated byte string)
//                ::= GV <object name>			# Guard variable for one-time initialization of static objects in a local scope
//                ::= T <call-offset> <base encoding>	# base is the nominal target function of thunk
//                ::= Tc <call-offset> <call-offset> <base encoding> # base is the nominal target function of thunk
//							# first call-offset is 'this' adjustment
//							# second call-offset is result adjustment
// <call-offset> ::= h <nv-offset> _
//               ::= v <v-offset> _
// <nv-offset> ::= <offset number>			# non-virtual base override
// <v-offset>  ::= <offset number> _ <virtual offset number> # virtual base override, with vcall offset
//
// <number> ::= [n] <decimal-integer>
//
// <decimal-integer> ::= 0
// 	             ::= 1|2|3|4|5|6|7|8|9 [<decimal-integer>]
//
// <name> ::= <nested-name>						# Starts with 'N'
//        ::= <unscoped-name>						# Starts with 'S', 'C', 'D', a digit or a lower case character.
//        ::= <unscoped-template-name> <template-args>			# idem
//        ::= <local-name>						# Starts with 'Z'
//
// <nested-name> ::= N [<CV-qualifiers>] <prefix> <unqualified-name> E
//               ::= N [<CV-qualifiers>] <template-prefix> <template-args> E
//
// <prefix> ::= <prefix> <unqualified-name>
//          ::= <template-prefix> <template-args>
//          ::= # empty
//          ::= <substitution>
//
// <template-prefix> ::= <prefix> <template unqualified-name>
//                   ::= <substitution>
//
// <unscoped-template-name> ::= <unscoped-name>
//                          ::= <substitution>
//
// <substitution> ::= S <seq-id> _
//                ::= S_
//
// <seq-id> ::= 0|1|2|3|4|5|6|7|8|9|A|B|C|D|E|F|G|H|I|J|K|L|M|N|O|P|Q|R|S|T|U|V|W|X|Y|Z [<seq-id>]	# Base 36 number
//
// <unscoped-name> ::= <unqualified-name>
//                 ::= St <unqualified-name>		# ::std::
//
// <unqualified-name> ::= <operator-name>				# Starts with lower case
//                    ::= <ctor-dtor-name>  				# Starts with 'C' or 'D'
//                    ::= <source-name>   				# Starts with a digit
//
// <operator-name> ::= nw				# new           
//                 ::= na				# new[]
//                 ::= dl				# delete        
//                 ::= da				# delete[]      
//                 ::= ng				# - (unary)     
//                 ::= ad				# & (unary)     
//                 ::= de				# * (unary)     
//                 ::= co				# ~             
//                 ::= pl				# +             
//                 ::= mi				# -             
//                 ::= ml				# *             
//                 ::= dv				# /             
//                 ::= rm				# %             
//                 ::= an				# &             
//                 ::= or				# |             
//                 ::= eo				# ^             
//                 ::= aS				# =             
//                 ::= pL				# +=            
//                 ::= mI				# -=            
//                 ::= mL				# *=            
//                 ::= dV				# /=            
//                 ::= rM				# %=            
//                 ::= aN				# &=            
//                 ::= oR				# |=            
//                 ::= eO				# ^=            
//                 ::= ls				# <<            
//                 ::= rs				# >>            
//                 ::= lS				# <<=           
//                 ::= rS				# >>=           
//                 ::= eq				# ==            
//                 ::= ne				# !=            
//                 ::= lt				# <             
//                 ::= gt				# >             
//                 ::= le				# <=            
//                 ::= ge				# >=            
//                 ::= nt				# !             
//                 ::= aa				# &&            
//                 ::= oo				# ||            
//                 ::= pp				# ++            
//                 ::= mm				# --            
//                 ::= cm				# ,             
//                 ::= pm				# ->*           
//                 ::= pt				# ->            
//                 ::= cl				# ()            
//                 ::= ix				# []            
//                 ::= qu				# ?             
//                 ::= sz				# sizeof        
//                 ::= sr				# scope resolution (::), see below        
//                 ::= cv <type>			# (cast)        
//                 ::= v <digit> <source-name>		# vendor extended operator
//
// <ctor-dtor-name> ::= C1				# complete object (in-charge) constructor
//                  ::= C2				# base object (not-in-charge) constructor
//                  ::= C3				# complete object (in-charge) allocating constructor
//                  ::= D0				# deleting (in-charge) destructor
//                  ::= D1				# complete object (in-charge) destructor
//                  ::= D2				# base object (not-in-charge) destructor
//
// <source-name> ::= <positive length number> <identifier>
//
// <local-name> := Z <function encoding> E <entity name> [<discriminator>]
//              := Z <function encoding> E s [<discriminator>]
// <discriminator> := _ <non-negative number>

//
// <type> ::= <builtin-type>
//        ::= <function-type>
//        ::= <class-enum-type>
//        ::= <array-type>
//        ::= <pointer-to-member-type>
//        ::= <template-param>
//        ::= <template-template-param> <template-args>
//        ::= <substitution> # See Compression below
//
// <type> ::= <CV-qualifiers> <type>
//        ::= P <type>   # pointer-to
//        ::= R <type>   # reference-to
//        ::= C <type>   # complex pair (C 2000)
//        ::= G <type>   # imaginary (C 2000)
//        ::= U <source-name> <type>     # vendor extended type qualifier
//
// <CV-qualifiers> ::= [r] [V] [K]       # restrict (C99), volatile, const
//
// <builtin-type> ::= v  # void
//                ::= w  # wchar_t
//                ::= b  # bool
//                ::= c  # char
//                ::= a  # signed char
//                ::= h  # unsigned char
//                ::= s  # short
//                ::= t  # unsigned short
//                ::= i  # int
//                ::= j  # unsigned int
//                ::= l  # long
//                ::= m  # unsigned long
//                ::= x  # long long, __int64
//                ::= y  # unsigned long long, __int64
//                ::= n  # __int128
//                ::= o  # unsigned __int128
//                ::= f  # float
//                ::= d  # double
//                ::= e  # long double, __float80
//                ::= g  # __float128
//                ::= z  # ellipsis
//                ::= u <source-name>    # vendor extended type
//
// <function-type> ::= F [Y] <bare-function-type> E
// <bare-function-type> ::= <signature type>+		# types are possible return type, then parameter types
//
// <class-enum-type> ::= <name>
//
// <array-type> ::= A <positive dimension number> _ <element type>
//              ::= A [<dimension expression>] _ <element type>
//
// <pointer-to-member-type> ::= M <class type> <member type>
//
// <template-param> ::= T_				# first template parameter
//                  ::= T <parameter-2 non-negative number> _
// <template-template-param> ::= <template-param>
//                           ::= <substitution>
//
// <template-args> ::= I <template-arg>+ E
// <template-arg> ::= <type>				# type or template
//                ::= L <type> <value number> E		# literal
//                ::= L_Z <encoding> E			# external name
//                ::= X <expression> E			# expression
//
// <expression> ::= <unary operator-name> <expression>
//              ::= <binary operator-name> <expression> <expression>
//              ::= <expr-primary>
//
// <expr-primary> ::= <template-param>
//                ::= L <type> <value number> E		# literal
//                ::= L <mangled-name> E			# external name
//

// Non-scope types:					PREFIX					POSTFIX
//
// v							void
// z							...
// b							bool
// f							float
// d							double
// e							long double
// w							wchar_t
// a							signed char
// h							unsigned char
// c							char
// t							unsigned short
// s							short
// j							unsigned
// i							int
// m							unsigned long
// l							long
// y							unsigned long long
// x							long long
// G<digits><global_name>				<global_name>
// P<type>						<prefix>*				<postfix>
// R<type>						<prefix>&				<postfix>
// PF<return_type><p1><p2>...<pN>E			<return_type> (*			)(<p1>, <p2>, ..., <pN>)
// RF<return_type><p1><p2>...<pN>E			<return_type> (&			)(<p1>, <p2>, ..., <pN>)
// M<scopetype>F<return_type><p1><p2>...<pN>E		<return_type> (<scopetype>::*		)(<p1>, <p2>, ..., <pN>)
// MK<scopetype>F<return_type><p1><p2>...<pN>E		<return_type> (<scopetype>::*		)(<p1>, <p2>, ..., <pN>) const
//
// Scope types:
//
// <digits><class_name>					<class_name>
// <digits>_GLOBAL__N_xxxx				{anonymous}
// <digits><name>I<ttype>I<ttype>...I<ttype>E		<name><<ttype>, <ttype>, ..., <ttype>>
//
// Possible scope types:
//
// K<type>						<prefix> const				<postfix>
// N<scopetype1>...<scopetypeN><type>E			<scopetype1>::...::<scopetypeN>::<type>
//


// Symbols
//
// <symbol>[<types>]						<symbol>
// N<scopetype1>...<scopetypeN><symbol>E[<types>]		<scopetype1>::...::<scopetypeN>::<symbol>[(<types>)]
// N[<scopetype1>...<scopetypeN>]<class-name>C1E<types>		[<scopetype1>::...::<scopetypeN>::]<class-name>::<class-name>(<types>)
// N[<scopetype1>...<scopetypeN>]<class-name>D1Ev		[<scopetype1>::...::<scopetypeN>::]<class-name>::~<class-name>()

