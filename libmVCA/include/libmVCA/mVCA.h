/* $Id: mVCA.h 1415 2010-11-02 16:00:49Z davidpiegdon $
 * vim: fdm=marker
 *
 * This file is part of libmVCA.
 *
 * libmVCA is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libmVCA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libmVCA.  If not, see <http://www.gnu.org/licenses/>.
 *
 * (c) 2009,2010 Lehrstuhl Softwaremodellierung und Verifikation (I2), RWTH Aachen University
 *           and Lehrstuhl Logik und Theorie diskreter Systeme (I7), RWTH Aachen University
 *           and David R. Piegdon <david-i2@piegdon.de>
 * Author: David R. Piegdon <david-i2@piegdon.de>
 *
 */

#ifndef __libmvca_mvca_h__
# define __libmvca_mvca_h__

#include <list>
#include <set>
#include <string>
#include <map>
#include <vector>

#include <libmVCA/pushdown.h>
#include <libmVCA/transition_function.h>

#include <libmVCA/serialize.h>

namespace libmVCA {

const char * libmVCA_version();

// NOTE: this implementation DOES NOT SUPPORT epsilon transitions.



class mVCA_run {
	public:
		std::list<int> prefix;
		int state;
		int m;

		mVCA_run()
		{ state = 0; m = 0; };

		mVCA_run(int state, int m)
		{ this->state = state; this->m = m; };
};

// for mVCA_run:
// NOTE: all comparison-functions don't care about the prefix! this way,
// we can easily make a breadth-first search and remember visited
// m/state tuples in a std::set<mVCA_run> this is used in mVCA::shortest_run
bool operator<(const mVCA_run first, const mVCA_run second);
bool operator==(const mVCA_run first, const mVCA_run second);
bool operator>(const mVCA_run first, const mVCA_run second);




// interface and common functions for nondeterministic_mVCA and deterministic_mVCA.
class mVCA {
	public: // types
		enum mVCA_derivate {
			DERIVATE_NONE = 0,

			DERIVATE_DETERMINISTIC = 1,
			DERIVATE_NONDETERMINISTIC = 2,

			DERIVATE_LAST_INVALID = 3
		};
	protected: // data
		unsigned int state_count;
		pushdown_alphabet alphabet;
		int initial_state;
		std::set<int> final_states;

		int m_bound; // there exist m_bound+1 transition_functions
//		transition_function :: implemented by deriving classes

		friend mVCA * construct_mVCA(unsigned int state_count, const pushdown_alphabet & alphabet, int initial_state, const std::set<int> & final_states, int m_bound, const std::map<int, std::map<int, std::map<int, std::set<int> > > > & transitions);

	public: // methods
		mVCA();
		virtual ~mVCA();

		// ALPHABET
		// set alphabet (will be copied, will erase all other information about former automaton)
		void set_alphabet(pushdown_alphabet & alphabet);
		void unset_alphabet();
		pushdown_alphabet get_alphabet() const;

		enum pushdown_direction alphabet_get_direction(int sigma) const;
		int get_alphabet_size() const;

		// STATES
		int get_state_count() const;

		int get_m_bound() const;

		// INITIAL/FINAL STATES
		int get_initial_state() const;
		std::set<int> get_initial_states() const;
		std::set<int> get_final_states() const;
		bool set_initial_state(unsigned int state);
		bool set_final_state(const std::set<int> & states);

		bool contains_initial_states(const std::set<int> & states) const;
		bool contains_final_states(const std::set<int> & states) const;

		// TRANSITIONS

		// get mappings of all transitions
		// the mapping works as follows: map[m][current_state][label] = { successor-states }.
		virtual void get_transition_map(std::map<int, std::map<int, std::map<int, std::set<int> > > > & postmap) const = 0;

		std::set<int> transition(int from, int & m, int label) const;
		virtual std::set<int> transition(const std::set<int> & from, int & m, int label) const = 0; // depends on transition function
		virtual bool endo_transition(std::set<int> & states, int & m, int label) const = 0; // depends on transition function

		std::set<int> run(const std::set<int> & from, int & m, std::list<int>::const_iterator word, std::list<int>::const_iterator word_limit) const;
		// get shortest run from a state in <from> to a state in <to>.
		// the run has to result in a configuration where the current state is a state in <to> and m is <to_m>.
		std::list<int> shortest_run(const std::set<int> & from, int m, const std::set<int> & to, int to_m, bool &reachable) const;
		// test if word is contained in language of automaton
		bool contains(const std::list<int> & word) const;
		bool contains(std::list<int>::const_iterator word, std::list<int>::const_iterator word_limit) const;
		// obtain shortest word in language resp. test if language is empty,
		std::list<int> get_sample_word(bool & is_empty) const;
		bool is_empty() const;

		void get_bounded_behaviour_graph(int m_bound, bool & f_is_deterministic, int & f_alphabet_size, int & f_state_count, std::set<int> & f_initial_states, std::set<int> & f_final_states, std::map<int, std::map<int, std::set<int> > > & f_transitions) const;

		///-----------------------------------

		bool lang_subset_of(mVCA & other, std::list<int> & counterexample); // both have to be deterministic. this implicitly calls complete_automaton() for this and other!
		bool lang_equal(mVCA & other, std::list<int> & counterexample); // both have to be deterministic. this implicitly calls complete_automaton() for this and other!
		bool lang_disjoint_to(const mVCA & other, std::list<int> & counterexample) const; // both have to be deterministic

		mVCA * lang_intersect(const mVCA & other) const; // both have to be deterministic
//		mVCA * lang_union(mVCA & other); // TODO: both may be nondeterministic. can be done efficiently.
						// just take care of transitions from/to initial state and merge both initial states.

		//bool lang_complement();
		//mVCA * determinize();

		///-----------------------------------

		// obtain id of unique derived class
		virtual enum mVCA_derivate get_derivate_id() const = 0;

		// format for serialization:
		// all values in NETWORK BYTE ORDER!
		// <serialized automaton>
		//	string length (not in bytes but in int32_t; excluding this length field)
		//	derivate id (enum mVCA_derivate)
		//	state_count
		//	<serialized alphabet>
		//	initial_state
		//	number of final states
		//	final_states[]
		//	m_bound
		//	<serialized derivate-specific data>
		// </serialized automaton>
		std::basic_string<int> serialize() const;
		bool deserialize(serial_stretch & serial);

		std::string visualize() const;

		// create a complete automaton from this automaton. i.e. add the implicit sink state
		// and transitions to it.
		void complete_automaton();
		// add (or overwrite, in case of deterministic) a transition
		virtual void add_transition(int m, int src, int label, int dst) = 0;
	protected:
		virtual std::basic_string<int32_t> serialize_derivate() const = 0;
		virtual bool deserialize_derivate(serial_stretch & serial) = 0;
		virtual std::string get_transition_dotfile() const = 0;


		mVCA * crossproduct(const mVCA & other) const;
		int crossproduct_state_match(const mVCA & other, int this_state, int other_state) const;
		int find_sink() const;

};






mVCA * construct_mVCA(	unsigned int state_count,
			int alphabet_size, const std::vector<int> & alphabet_directions,
			int initial_state,
			const std::set<int> & final_states,
			int m_bound,
			const std::map<int, std::map<int, std::map<int, std::set<int> > > > & transitions
			// transitions: m -> state -> sigma -> states
		);

mVCA * construct_mVCA(	unsigned int state_count,
			const pushdown_alphabet & alphabet,
			int initial_state,
			const std::set<int> & final_states,
			int m_bound,
			const std::map<int, std::map<int, std::map<int, std::set<int> > > > & transitions
			// transitions: m -> state -> sigma -> states
		);


mVCA * deserialize_mVCA(serial_stretch & serial);



}; // end of namespace libmVCA.

#endif // __libmvca_mvca_h__

