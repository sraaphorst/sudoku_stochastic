/**
 * GenSudokuBoardHCPopulator.cpp
 *
 * By Sebastian Raaphorst, 2018.
 */

#pragma once

#include <iterator>
#include <random>
#include <stdexcept>

#include <boost/format.hpp>

#include "AscenderPopulator.h"
#include "DefaultMethods.h"
#include "GenSudokuBoardPopulator.h"
#include "RNG.h"

namespace vorpal::gensudoku {
    /**
     * The implementation for the most basic form of hill climbing.
     * @tparam N the size parameter
     */
    template<size_t N = 3,
            const auto NN = N * N,
            const auto BoardSize = NN  * NN>
    class GenSudokuBoardHCPopulator:
            public virtual GenSudokuBoardPopulator<N>,
            public virtual stochastic::AscenderPopulator<GenSudokuBoard<N>> {

        constexpr static double DEFAULT_N_PROBABILITY = 0.3;

        // The probability in mutating a cell that can be mutated.
        const double prob_n;

    public:
        __GENSUDOKUBOARD_POPULATOR_GENERIC_CONSTRUCTORS(GenSudokuBoardHCPopulator)

        explicit GenSudokuBoardHCPopulator(const data_type &partial_board, double prob_n = DEFAULT_N_PROBABILITY):
            GenSudokuBoardPopulator<N>{partial_board}, prob_n{prob_n} {}

        explicit GenSudokuBoardHCPopulator(data_type &&partial_board, double prob_n = DEFAULT_N_PROBABILITY):
            GenSudokuBoardPopulator<N>{partial_board}, prob_n{prob_n} {}

        pointer_type generate() noexcept override {
            auto board = std::make_unique<GenSudokuBoard<N>>(this->partial_board);

            // Fill the empty slots on the board with any valid number.
            auto &gen = stochastic::RNG::getGenerator();

            for (size_t pos = 0; pos < BoardSize; ++pos) {
                if ((*board)[pos] == 0) {
                    const auto value = std::uniform_int_distribution<size_t>(0, this->cell_candidates[pos].size() - 1)(gen);
                    (*board)[pos] = this->cell_candidates[pos][value];
                }
            }

            return board;
        }

//        pointer_type generate() noexcept override {
//            // For each row, shuffle the missing entries and distribute them amongst the empty positions.
//            auto board = std::make_unique<GenSudokuBoard<N>>(this->partial_board);
//            for (size_t row = 0; row < NN; ++row)
//                this->fillRow(board, row);
//            return std::move(board);
//        }

        virtual pointer_type selectedNeighbour(int iteration, pointer_type &current) const override {
            auto neighbour = std::move(nOperator(current));
            if (neighbour->fitness() > current->fitness())
                return std::move(neighbour);
            else
                return std::move(current);
        }

    private:
        pointer_type nOperator(const pointer_type &current_board) const {
            auto &gen = stochastic::RNG::getGenerator();
            std::uniform_real_distribution<> distribution;

            // Mutate current by the N-operator, which runs a probability check for every non-fixed cell.
            // If the probability check succeeds, it attempts to decreatse or increase the value of the cel
            // by a jump.
            pointer_type neighbour_board = std::make_unique<data_type>(*current_board);

            for (size_t pos = 0; pos < BoardSize; ++pos) {
                const auto &candidates = this->cell_candidates[pos];
                if (candidates.size() > 1 && distribution(gen) < prob_n) {
                    const auto endIter = std::end(candidates);
                    auto iter = std::find(std::begin(candidates), endIter, (*current_board)[pos]);
                    if (iter == endIter)
                        throw std::logic_error((boost::format("board illegal state: value %1% found at (%2%,%3%))")
                            % ((*current_board)[pos]) % (pos / NN) % (pos % NN)).str());

                    // Legal state: Determine if we are going to move up or down.
                    int pos_modifier = distribution(gen) < 0.5 ? -1 : 1;
                    auto idx = (std::distance(std::begin(candidates), iter) + pos_modifier + candidates.size()) % candidates.size();
                    (*neighbour_board)[pos] = candidates[idx];
                }
            }

            return neighbour_board;
        }
    };

    using SudokuBoardHCPopulator = GenSudokuBoardHCPopulator<>;
}
/**
 * A modification of a hill climbing algorithm as discovered by:
 *
 * Mohammed Azmi Al-Betar. 2017. $\beta$-Hill climbing: an exploratory local search.
 * Neural Computing and Applications 28, 1 (January 2017), pp. 153-168.
 *
 * M. A. Al-Betar, M. A. Awadallah, A. L. Bolaji and B. O. Alijla, $\beta$-Hill Climbing Algorithm for Sudoku Game.
 * 2017 Palestinian International Conference on Information and Communication Technology (PICICT), Gaza, Palestine, 2017, pp. 84-88.
 *
 * Note that by omitting the beta-function, a regular hill climber is achieved.
 */