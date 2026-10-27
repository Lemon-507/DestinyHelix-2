#include "reward_viewmodel.h"

#include "game_manager.h"
#include "utils/item_tooltips.h"

namespace {

RewardViewModel::RewardType toRewardType(RewardOfferKind kind)
{
    switch (kind) {
    case RewardOfferKind::Potion:
        return RewardViewModel::RewardType::Potion;
    case RewardOfferKind::Card:
        return RewardViewModel::RewardType::Card;
    case RewardOfferKind::Relic:
        return RewardViewModel::RewardType::Relic;
    case RewardOfferKind::Gold:
    default:
        return RewardViewModel::RewardType::Gold;
    }
}

QString tooltipFor(const RewardOfferData &offer)
{
    switch (offer.kind) {
    case RewardOfferKind::Potion:
        return ItemTooltips::forPotion(offer.itemId);
    case RewardOfferKind::Card:
        return ItemTooltips::forCard(offer.itemId);
    case RewardOfferKind::Relic:
        return ItemTooltips::forRelic(offer.itemId);
    case RewardOfferKind::Gold:
    default:
        return {};
    }
}

} // namespace

RewardViewModel::RewardViewModel(GameManager *gameManager, QObject *parent)
    : QObject(parent)
    , m_gameManager(gameManager)
{
    if (m_gameManager != nullptr) {
        m_gameManager->rollRewards();
    }
    refreshFromGameManager();
}

const QVector<RewardViewModel::RewardOffer> &RewardViewModel::offers() const
{
    return m_offers;
}

void RewardViewModel::refreshFromGameManager()
{
    m_offers.clear();
    if (m_gameManager == nullptr) {
        return;
    }

    for (const RewardOfferData &data : m_gameManager->rewardOffers()) {
        RewardOffer offer;
        offer.type = toRewardType(data.kind);
        offer.buttonText = data.displayName;
        offer.granted = data.granted;
        offer.claimed = data.claimed;
        offer.goldAmount = data.goldAmount;
        offer.itemId = data.itemId;
        if (data.granted && !data.itemId.isEmpty()) {
            offer.tooltip = tooltipFor(data);
        }
        m_offers.append(offer);
    }
    emit offersChanged();
}

bool RewardViewModel::claimReward(RewardType type)
{
    if (m_gameManager == nullptr) {
        return false;
    }

    const QVector<RewardOfferData> &offers = m_gameManager->rewardOffers();
    int offerIndex = -1;
    for (int i = 0; i < offers.size(); ++i) {
        if (toRewardType(offers.at(i).kind) == type) {
            offerIndex = i;
            break;
        }
    }
    if (offerIndex < 0 || !m_gameManager->claimReward(offerIndex)) {
        return false;
    }

    refreshFromGameManager();
    return true;
}
