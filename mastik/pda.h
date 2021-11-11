/*
 * Copyright 2016 CSIRO
 *
 * This file is part of Mastik.
 *
 * Mastik is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mastik is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mastik.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PDA_H__
#define __PDA_H__ 1


/*
 * Perfoms a performance degradation attack.
 * See: * T. Allan, B.B. Brumley, K. Falkner, J. van de Pol and 
 * Y. Yarom, "Amplifying Side Channels Through Performance Degradation," 
 * ACSAC 2016 (eprint 2015/1141)
 *
 * pda_t abstracts over a single attacking thread (currently implemented as
 * a child process). Each attack targets a set of addresses which it continuosly
 * evicts from the cache. 
 * To ensure attack effectiveness, the target set cannot be changed while the
 * attack is active.  Any changes to the target set becomes effective in the
 * activation following the change.
 */

typedef struct pda *pda_t;



/*
 * Prepares a new instance of a performance degradation attack
 */
pda_t pda_prepare();


/*
 * Releases all resources associated with a performace degradation attack
 */
void pda_release(pda_t pda);

/*
 * Adds an address to be targeted in a performance degradation attack
 * The address will be targeted from the next activation of the attack.
 * Does not affect an active attack.
 */
int pda_target(pda_t pda, void *adrs);

/*
 * Removes an address from a performance degradation attack
 * The address will be removed from the next activation of the attack.
 * Does not affect an active attack.
 */
int pda_untarget(pda_t pda, void *adrs);

/*
 * Returns the list of addresses to be targeted in the next activation of the attack.
 */
int pda_gettargetset(pda_t pda, void **adrss, int nlines);

/*
 * Randomises the order of accessing targeted addresses in the attack.  Effective
 * starting the next activation.
 */
void pda_randomise(pda_t pda);

/*
 * Activates a performance degradation attack
 *
 * If executed on an active attack, the attack may be stopped and restarted to accommodate
 * any changes to the target set.
 */
void pda_activate(pda_t pda);

/*
 * Deactivates a performance degradation attack.
 */
void pda_deactivate(pda_t pda);

/*
 * Returns true if the attack is currently active.
 */
int pda_isactive(pda_t pda);


#endif // __PDA_H__

