/* HackTM - C++ implementation of Numenta Cortical Learning Algorithm.
 * Copyright (c) 2010-2011 Gianluca Guida <glguida@gmail.com>
 *  
 * This software is released under the Numenta License for
 * Non-Commercial Use. You should have received a copy of the Numenta
 * License for Non-Commercial Use with this software (usually
 * contained in a file called LICENSE.TXT). If you don't, you can
 * download it from 
 * http://numenta.com/about-numenta/Numenta_Non-Commercial_License.txt.
 */

#include "HackTM.h"
#include "DendriteSegment.h"

typedef std::list<struct synapse *>::iterator synapse_iterator;

namespace hacktm {

  void
  DendriteSegment::computeState(const htmtime_t t)
  {
    unsigned learn = 0;
    unsigned active = 0;
    __state[t] = inactiveState;

    synapse_iterator it;
    for ( it = __potentialSynapses.begin(); 
	  it != __potentialSynapses.end(); it++ ) {
      struct synapse *syn = *it;
      if ( syn->perm < htmconfig::connectedPerm ) {
	continue;
      }

      if ( __cellstate->activeState(syn->id, t) )
	active++;
      if ( __cellstate->learnState(syn->id) )
	learn++;
    }

    if ( learn > htmconfig::activationThreshold ) {
      __state[t] = learnState;
      __activity[t] = learn;
    } else if ( active > htmconfig::activationThreshold ) {
      __state[t] = activeState;
      __activity[t] = active;
    } else {
      __state[t] = inactiveState;
      __activity[t] = 0;
    }
  }

  unsigned
  DendriteSegment::getMatchingSynapses(const htmtime_t t) const
  {
    unsigned active = 0;
    std::list<struct synapse *>::const_iterator it;
    for ( it = __potentialSynapses.begin();
	  it != __potentialSynapses.end(); it++ ) {
      struct synapse *syn = *it;

      if ( __cellstate->activeState(syn->id, t) 
	   && syn->perm >= htmconfig::initialPerm )
	active++;
    }

    if ( active > htmconfig::minThreshold )
      return active;

    return 0;
  }

  void 
  DendriteSegment::getActiveSynapses(const htmtime_t t, std::list<id_t> &activeSynapses) const
  {
    std::list<struct synapse *>::const_iterator it;
    for ( it = __potentialSynapses.begin();
	  it != __potentialSynapses.end(); it++ ) {
      struct synapse *syn = *it;

      /* push_back here is important: it ensures that activeSynapses
	 are ordered by ID, given the way they're added! */
      if ( __cellstate->activeState(syn->id, t) && syn->perm > 0.0f )
	activeSynapses.push_back(syn->id);
    }
  }

  static void __decrementSynapse(struct synapse *s)
  {
    s->perm = std::max(0.0f, s->perm - htmconfig::permanenceDec);
  }

  static void __incrementSynapse(struct synapse *s)
  {
    s->perm = std::min(1.0f, s->perm + htmconfig::permanenceInc);
  }

  static bool __syn_comp(struct synapse *syn1, struct synapse *syn2)
  {
    return syn1->id < syn2->id;
  }

  void
  DendriteSegment::addSynapses(std::list<id_t> &newSynapses)
  {
    std::list<id_t>::iterator newSynIt;
    struct synapse syn2;
    synapse_iterator it;
    for ( newSynIt = newSynapses.begin(); newSynIt != newSynapses.end(); newSynIt++ ) {
      syn2.id = *newSynIt;
      it = std::lower_bound(__potentialSynapses.begin(), __potentialSynapses.end(), &syn2, __syn_comp);

      /* getRandomLearnCells does not guaratee us that synapse aren't there already. */
      if ( it != __potentialSynapses.end() 
	   && (*it)->id == *newSynIt ) {
	__incrementSynapse(*it);
	continue;
      }

      /* Insert new synapse here. */
      synapse *newsyn = new synapse;
      newsyn->id = *newSynIt;
      newsyn->perm = htmconfig::initialPerm;
      __potentialSynapses.insert(it, newsyn);
    }
  }

  void
  DendriteSegment::synapseReinforcement(std::list<id_t> &activeSynapses, bool sequence, bool positive)
  {
    std::list<id_t>::iterator actSynIterator = activeSynapses.begin();
    synapse_iterator synapseIterator;

    if ( positive && sequence )
      __sequence = true;

    for ( synapseIterator = __potentialSynapses.begin();
	  synapseIterator != __potentialSynapses.end(); synapseIterator++) {
      struct synapse *syn = *synapseIterator;
	
      if ( positive && syn->id < *actSynIterator ) {
	__decrementSynapse(syn);
	continue;
      }

      if ( syn->id == *actSynIterator ) {
	if ( positive )
	  __incrementSynapse(syn);
	else
	  __decrementSynapse(syn);
	++actSynIterator;
	if ( actSynIterator == activeSynapses.end() )
	  break;
      }
    }
    /* activeSynapses scanned */

    if ( !positive )
      return;

    for (; synapseIterator != __potentialSynapses.end(); synapseIterator++) {
      struct synapse *syn = *synapseIterator;
      __decrementSynapse(syn);
    }
  }

} // namespace
