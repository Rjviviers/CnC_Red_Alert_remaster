/*
**	Command & Conquer Red Alert(tm) - modern port
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
**	ENUMOPS.H -- per-enum operator definitions (session S10a, ADR-0021).
**
**	JSHELL.H has generic templates for "++", "|", "&" and "~" on enumerations. The original
**	headers then "instantiate" them per enum with a bare declaration such as
**
**		inline ThreatType operator |(ThreatType, ThreatType);
**
**	Watcom 10.6 treated that as a request to generate the template for that signature.
**	Standard C++ treats it as a separate, non-template function that is preferred by overload
**	resolution and never defined, so every use is an unresolved symbol at link time.
**
**	These macros replace those declarations. Under Watcom they expand to the original
**	declaration. Elsewhere they define the function with a body copied from the JSHELL.H
**	template, so overload resolution (the candidate set is unchanged) and the result are
**	identical. The bodies are copied rather than forwarded to the template because some
**	headers (DEFINES.H via QUEUE.H) can be compiled without JSHELL.H in view.
*/

#ifndef ENUMOPS_H
#define ENUMOPS_H

#ifdef __WATCOMC__

#define	RA_ENUM_POSTINC(T)	inline T operator++(T &, int);
#define	RA_ENUM_BITOPS(T)	\
	inline T operator |(T, T);	\
	inline T operator &(T, T);	\
	inline T operator ~(T);

#else

/* Same body as "template<class T> inline T operator ++(T & a, int)" in JSHELL.H. */
#define	RA_ENUM_POSTINC(T)	\
	inline T operator++(T & a, int) {T aa = a; a = (T)((int)a + (int)1); return(aa);}

/* Same bodies as the "|", "&" and "~" templates in JSHELL.H. */
#define	RA_ENUM_BITOPS(T)	\
	inline T operator |(T t1, T t2) {return((T)((int)t1 | (int)t2));}	\
	inline T operator &(T t1, T t2) {return((T)((int)t1 & (int)t2));}	\
	inline T operator ~(T t1) {return((T)(~(int)t1));}

#endif

#endif
