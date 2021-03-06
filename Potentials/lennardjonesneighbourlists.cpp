#include "lennardjonesneighbourlists.h"
#include "../celllist.h"
#include "../neighbourlist.h"
#include "../system.h"

using std::cout;
using std::endl;

LennardJonesNeighbourLists::LennardJonesNeighbourLists(std::vector<real>  systemSize,
                           real               rCut,
                           System*              system) :
    LennardJonesNeighbourLists(1.0, 3.405, systemSize, rCut, rCut, system) {
}

LennardJonesNeighbourLists::LennardJonesNeighbourLists(real               epsilon,
                           real               sigma,
                           std::vector<real>  systemSize,
                           real               rCut,
                           real               neighbourCut,
                           System*              system) :
    Potential(system) {

    m_epsilon           = epsilon;
    m_sigma             = sigma;
    m_sigma6            = sigma*sigma*sigma*sigma*sigma*sigma;
    m_24epsilon         = 24*m_epsilon;
    m_4epsilonSigma6    = 4*m_epsilon*m_sigma6;
    m_systemSize        = systemSize;
    m_systemSizeHalf    = {systemSize[0]/2.0f, systemSize[1]/2.0f, systemSize[2]/2.0f};
    m_rCut              = rCut;
    m_rCut2             = rCut * rCut;
    m_neighbourCut      = neighbourCut;
    m_neighbourList     = new NeighbourList(m_system, m_rCut, m_neighbourCut, m_systemSize);
    m_cellList          = m_neighbourList->getCellList();
    real r2           = 1.0f / m_rCut2;
    m_potentialAtCut    = 4*m_epsilon * r2*r2*r2 * m_sigma6 * (m_sigma6*r2*r2*r2-1);
}

LennardJonesNeighbourLists::LennardJonesNeighbourLists( real               epsilon,
                                                        real               sigma,
                                                        real               rCut,
                                                        real               neighbourCut,
                                                        System*            system) :
        LennardJonesNeighbourLists(epsilon,
                                   sigma,
                                   system->getSystemSize(),
                                   rCut,
                                   neighbourCut,
                                   system) {

}

void LennardJonesNeighbourLists::computeForces() {

    if (m_timeStepsSinceLastCellListUpdate == -1 ||
        m_timeStepsSinceLastCellListUpdate >= 20) {
        m_timeStepsSinceLastCellListUpdate = 1;
        m_neighbourList->constructNeighbourLists();
    }
    m_timeStepsSinceLastCellListUpdate += 1;
    setForcesToZero();
    m_potentialEnergy       = 0;
    m_pressure              = 0;
    real dr2              = 0;
    real dr[3]            = {0,0,0};


    for (int i=0; i<m_system->getN(); i++) {
        Atom* atom1 = at(m_system->getAtoms(), i);
        vector<Atom*> neighbours = m_neighbourList->getNeighbours(atom1->getIndex());
        for (unsigned int j=0; j<neighbours.size(); j++) {
            Atom* atom2 = at(neighbours, j);

            dr2 = 0;
            for (int k=0; k < 3; k++) {
                dr[k] = at(atom1->getPosition(),k) - at(atom2->getPosition(),k);
                if (dr[k] > at(m_systemSizeHalf,k)) {
                    dr[k] = dr[k] - at(m_systemSize,k);
                } else if (dr[k] < -at(m_systemSizeHalf,k)) {
                    dr[k] = dr[k] + at(m_systemSize,k);
                }
                dr2 += dr[k]*dr[k];
            }

            if (atom1 != atom2) {
                const real r2         = 1.0f / dr2;
                const real r6         = r2*r2*r2;
                const real sigma6r6   = m_sigma6 * r6;
                const int cut           = (dr2 < m_rCut2);
                const real f          = -m_24epsilon * sigma6r6 *
                                          (2*sigma6r6 - 1) * r2 * cut;
                m_potentialEnergy      += (m_4epsilonSigma6 * r6 *
                                           (sigma6r6 - 1) - m_potentialAtCut) * cut;

                for (int k=0; k < 3; k++) {
                    const real df = f * dr[k];
                    atom1->addForce(-df, k);
                    atom2->addForce( df, k);
                    //m_pressure += f * std::sqrt(dr2) * r2;
                }
            }
        }
    }
}



real LennardJonesNeighbourLists::computePotential() {
    return m_potentialEnergy;
}

