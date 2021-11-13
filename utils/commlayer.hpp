/**************************************************************************//**
 * @file commlayer.hpp
 * @author Alexis Cabana-Loriaux
 * 
 * @brief Contains class for sublayers of the Spacepacket layer
 * 
 ******************************************************************************/
#ifndef COMMUNICATIONLAYER_HPP
#define COMMUNICATIONLAYER_HPP

#include "utils/buffer.hpp"

/**
 * @brief 
 */
class ICommunicationLayer
{
public:
    virtual void connectUpperLayer(ICommunicationLayer& upper_layer) {
        upper = &upper_layer;
        upper_layer.lower = this;
    }

protected:
    void pushToUpperLayer(const IBuffer& bytes) {
        if(upper != nullptr) {
            upper->receiveFromSubLayer(bytes);
        }
    }

    void pushToSubLayer(const IBuffer& bytes) {
        if(lower != nullptr) {
            lower->receiveFromUpperLayer(bytes);
        }
    }

private:
    virtual void receiveFromSubLayer(const IBuffer& bytes) = 0;
    virtual void receiveFromUpperLayer(const IBuffer& bytes) = 0;

    ICommunicationLayer* upper = nullptr;
    ICommunicationLayer* lower = nullptr;
};

#endif //COMMUNICATIONLAYER_HPP
