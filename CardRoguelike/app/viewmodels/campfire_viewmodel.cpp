#include "campfire_viewmodel.h"

#include "game_manager.h"

CampfireViewModel::CampfireViewModel(GameManager *gameManager, QObject *parent)
    : QObject(parent)
    , m_gameManager(gameManager)
{
}

bool CampfireViewModel::canUpgrade() const
{
    return m_gameManager != nullptr && m_gameManager->canUpgradeAtCampfire();
}

void CampfireViewModel::requestRest()
{
    emit restChosen();
}

void CampfireViewModel::requestUpgrade()
{
    emit upgradeChosen();
}
