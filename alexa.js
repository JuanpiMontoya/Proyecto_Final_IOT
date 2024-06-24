const Alexa = require('ask-sdk-core');
const AWS = require('aws-sdk');
const DynamoDB = new AWS.DynamoDB.DocumentClient();
const IotData = new AWS.IotData({ endpoint: 'a2mnkqceizplps-ats.iot.us-east-1.amazonaws.com' });

function getShadowPromise(params) {
    return new Promise((resolve, reject) => {
        IotData.getThingShadow(params, (err, data) => {
            if (err) {
                console.log(err, err.stack);
                reject(`Failed to get thing shadow ${err.errorMessage}`);
            } else {
                resolve(JSON.parse(data.payload));
            }
        });
    });
}

async function getUserThings(userId) {
    const params = {
        TableName: 'UserThings',
        Key: { userId: userId }
    };
    try {
        const data = await DynamoDB.get(params).promise();
        return data.Item ? data.Item.thingNames : [];
    } catch (error) {
        console.error(`Error getting item from DynamoDB: ${error}`);
        return [];
    }
}

async function addUserThing(userId, thingName) {
    const currentThings = await getUserThings(userId);
    if (!currentThings.includes(thingName)) {
        currentThings.push(thingName);
    }

    const params = {
        TableName: 'UserThings',
        Item: {
            userId: userId,
            thingNames: currentThings
        }
    };
    try {
        await DynamoDB.put(params).promise();
    } catch (error) {
        console.error(`Error putting item to DynamoDB: ${error}`);
    }
}

function calculateWaterLimit(area, plantType) {
    let waterPerSquareMeter;
    switch (plantType) {
        case 'cesped':
            waterPerSquareMeter = 6; // 6 litros por metro cuadrado
            break;
        case 'flores':
            waterPerSquareMeter = 5;  // 5 litros por metro cuadrado
            break;
        case 'cultivos':
            waterPerSquareMeter = 8; // 8 litros por metro cuadrado
            break;
        case 'arboles':
            waterPerSquareMeter = 15; // 15 litros por metro cuadrado
            break;
        case 'arbustos':
            waterPerSquareMeter = 4; // 4 litros por metro cuadrado
            break;
        case 'vegetales':
            waterPerSquareMeter = 10; // 10 litros por metro cuadrado
            break;
        default:
            waterPerSquareMeter = 6; // Valor por defecto
    }
    return area * waterPerSquareMeter;
}

function calculateHumidityPercentage(plantType) {
    let humidityPercentage;
    switch (plantType) {
        case 'cesped':
            humidityPercentage = 10; // 30-50% para césped, valor promedio 40%
            break;
        case 'flores':
            humidityPercentage = 50;  // 40-60% para flores, valor promedio 50%
            break;
        case 'cultivos':
            humidityPercentage = 60; // 50-70% para cultivos, valor promedio 60%
            break;
        case 'arboles':
            humidityPercentage = 45; // 30-60% para árboles, valor promedio 45%
            break;
        case 'arbustos':
            humidityPercentage = 50; // 40-60% para arbustos, valor promedio 50%
            break;
        case 'vegetales':
            humidityPercentage = 65; // 60-70% para vegetales, valor promedio 65%
            break;
        default:
            humidityPercentage = 50; // Valor por defecto
    }
    return humidityPercentage;
}


const LaunchRequestHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'LaunchRequest';
    },
    handle(handlerInput) {
        const speakOutput = 'Bienvenido a Riego inteligente, ¿cómo se llama tu dispositivo?';
        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt(speakOutput)
            .getResponse();
    }
};

const CaptureThingNameIntentHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest' &&
            Alexa.getIntentName(handlerInput.requestEnvelope) === 'CaptureThingNameIntent';
    },
    async handle(handlerInput) {
        const thingName = Alexa.getSlotValue(handlerInput.requestEnvelope, 'thingName');
        const sessionAttributes = handlerInput.attributesManager.getSessionAttributes();
        sessionAttributes.thingName = thingName;

        // Guardar en DynamoDB para asegurar persistencia
        const userId = handlerInput.requestEnvelope.session.user.userId;
        await addUserThing(userId, thingName);

        const speakOutput = `El nombre de tu dispositivo es ${thingName}. ¿Cuál es el área del terreno que deseas regar en metros cuadrados?`;
        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt(speakOutput)
            .getResponse();
    }
};

const CaptureAreaIntentHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest' &&
            Alexa.getIntentName(handlerInput.requestEnvelope) === 'CaptureAreaIntent';
    },
    handle(handlerInput) {
        const area = Alexa.getSlotValue(handlerInput.requestEnvelope, 'area');
        const sessionAttributes = handlerInput.attributesManager.getSessionAttributes();
        sessionAttributes.area = area;

        const speakOutput = 'Perfecto, ¿qué deseas regar? ¿césped, flores, cultivos, árboles, arbustos o vegetales?';
        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt(speakOutput)
            .getResponse();
    }
};

const CapturePlantTypeIntentHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest' &&
            Alexa.getIntentName(handlerInput.requestEnvelope) === 'CapturePlantTypeIntent';
    },
    async handle(handlerInput) {
        const plantType = Alexa.getSlotValue(handlerInput.requestEnvelope, 'plantType');
        const sessionAttributes = handlerInput.attributesManager.getSessionAttributes();
        const area = sessionAttributes.area;
        let thingName = sessionAttributes.thingName;

        if (!thingName) {
            const userId = handlerInput.requestEnvelope.session.user.userId;
            const userThings = await getUserThings(userId);
            if (userThings.length === 1) {
                thingName = userThings[0];
            } else {
                const speakOutput = 'Tienes varios dispositivos. Por favor, especifica el nombre del dispositivo que quieres controlar.';
                return handlerInput.responseBuilder
                    .speak(speakOutput)
                    .reprompt(speakOutput)
                    .getResponse();
            }
        }

        const waterLimit = calculateWaterLimit(area, plantType);
        const humidityPercentage = calculateHumidityPercentage(plantType); 
        const params = {
            thingName: thingName,
            payload: JSON.stringify({ state: { desired: { water_limit: waterLimit, area: area, plant_type: plantType, recommended_humidity: humidityPercentage, info: 1} } })
        };

        try {
            await IotData.updateThingShadow(params).promise();
        } catch (err) {
            console.log(err);
        }
        

        const speakOutput = `Para un área de ${area} metros cuadrados regando ${plantType}, la cantidad de agua recomendada es ${waterLimit} litros. ¿Qué te gustaría hacer ahora? Puedes decir 'abrir la válvula', 'cerrar la válvula' o 'consultar el estado'.`;
        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt(speakOutput)
            .getResponse();
    }
};

const StateIntentHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest' &&
            Alexa.getIntentName(handlerInput.requestEnvelope) === 'StateIntent';
    },
    async handle(handlerInput) {
        const thingNameSlot = handlerInput.requestEnvelope.request.intent.slots.thingName.value;
        let thingName = thingNameSlot;

        const userId = handlerInput.requestEnvelope.session.user.userId;
        const userThings = await getUserThings(userId);

        if (!thingNameSlot && userThings.length === 1) {
            thingName = userThings[0];
        } else if (!thingNameSlot && userThings.length > 1) {
            const speakOutput = `Tienes varios dispositivos. Por favor, especifica el nombre del dispositivo que quieres controlar.`;
            return handlerInput.responseBuilder
                .speak(speakOutput)
                .reprompt(speakOutput)
                .getResponse();
        } else if (!userThings.includes(thingName)) {
            const speakOutput = `El dispositivo ${thingName} no está registrado. Por favor, proporciona un nombre de dispositivo válido.`;
            return handlerInput.responseBuilder
                .speak(speakOutput)
                .reprompt(speakOutput)
                .getResponse();
        }

        const ShadowParams = {
            thingName: thingName,
        };

        let speakOutput = 'No se pudo consultar el estado de la válvula, por favor intente más tarde.';
        try {
            const result = await getShadowPromise(ShadowParams);
            const watering = result.state.reported.watering;

            if (watering == 0) {
                speakOutput = 'La válvula está cerrada';
            } else if (watering == 1) {
                speakOutput = 'La válvula está abierta';
            }
        } catch (err) {
            console.log('Error al obtener el estado del dispositivo:', err);
        }

        speakOutput += '. ¿Qué te gustaría hacer ahora? Puedes decir "abrir la válvula" o "cerrar la válvula".';

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt(speakOutput)
            .getResponse();
    }
};

const CloseValveIntentHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest' &&
            Alexa.getIntentName(handlerInput.requestEnvelope) === 'CloseValveIntent';
    },
    async handle(handlerInput) {
        const thingNameSlot = handlerInput.requestEnvelope.request.intent.slots.thingName.value;
        let thingName = thingNameSlot;

        const userId = handlerInput.requestEnvelope.session.user.userId;
        const userThings = await getUserThings(userId);

        if (!thingNameSlot && userThings.length === 1) {
            thingName = userThings[0];
        } else if (!thingNameSlot && userThings.length > 1) {
            const speakOutput = `Tienes varios dispositivos. Por favor, especifica el nombre del dispositivo que quieres controlar.`;
            return handlerInput.responseBuilder
                .speak(speakOutput)
                .reprompt(speakOutput)
                .getResponse();
        } else if (!userThings.includes(thingName)) {
            const speakOutput = `El dispositivo ${thingName} no está registrado. Por favor, proporciona un nombre de dispositivo válido.`;
            return handlerInput.responseBuilder
                .speak(speakOutput)
                .reprompt(speakOutput)
                .getResponse();
        }

        const params = {
            thingName: thingName,
            payload: JSON.stringify({ state: { desired: { watering: 0, info: 1} } })
        };

        try {
            await IotData.updateThingShadow(params).promise();
            const speakOutput = 'La válvula se ha cerrado. ¿Qué te gustaría hacer ahora? Puedes decir "abrir la válvula" o "consultar el estado".';
            return handlerInput.responseBuilder
                .speak(speakOutput)
                .reprompt(speakOutput)
                .getResponse();
        } catch (err) {
            console.log(err);
            const speakOutput = 'Hubo un problema al cerrar la válvula, por favor intenta nuevamente.';
            return handlerInput.responseBuilder
                .speak(speakOutput)
                .reprompt(speakOutput)
                .getResponse();
        }
    }
};

const OpenValveIntentHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest' &&
            Alexa.getIntentName(handlerInput.requestEnvelope) === 'OpenValveIntent';
    },
    async handle(handlerInput) {
        const thingNameSlot = handlerInput.requestEnvelope.request.intent.slots.thingName.value;
        let thingName = thingNameSlot;

        const userId = handlerInput.requestEnvelope.session.user.userId;
        const userThings = await getUserThings(userId);

        if (!thingNameSlot && userThings.length === 1) {
            thingName = userThings[0];
        } else if (!thingNameSlot && userThings.length > 1) {
            const speakOutput = `Tienes varios dispositivos. Por favor, especifica el nombre del dispositivo que quieres controlar.`;
            return handlerInput.responseBuilder
                .speak(speakOutput)
                .reprompt(speakOutput)
                .getResponse();
        } else if (!userThings.includes(thingName)) {
            const speakOutput = `El dispositivo ${thingName} no está registrado. Por favor, proporciona un nombre de dispositivo válido.`;
            return handlerInput.responseBuilder
                .speak(speakOutput)
                .reprompt(speakOutput)
                .getResponse();
        }

        // Obtener el estado actual del shadow del dispositivo
        try {
            const shadowParams = {
                thingName: thingName
            };

            const shadowData = await IotData.getThingShadow(shadowParams).promise();
            const shadowState = JSON.parse(shadowData.payload);
            const reportedMoisture = shadowState.state.reported.moisture;

            if (reportedMoisture === 'sufficient') {
                const speakOutput = `La humedad del suelo ya es suficiente. Se recomienda cerrar la válvula para evitar el exceso de riego. ¿Qué te gustaría hacer ahora? Puedes decir "cerrar la válvula" o "consultar el estado"`;
                return handlerInput.responseBuilder
                    .speak(speakOutput)
                    .reprompt(speakOutput)
                    .getResponse();
            }
        } catch (err) {
            console.log('Error al obtener el shadow:', err);
            const speakOutput = 'Hubo un problema al consultar el estado del dispositivo, por favor intenta nuevamente.';
            return handlerInput.responseBuilder
                .speak(speakOutput)
                .reprompt(speakOutput)
                .getResponse();
        }

        // Si la humedad no es suficiente, proceder a abrir la válvula
        const params = {
            thingName: thingName,
            payload: JSON.stringify({ state: { desired: { watering: 1, info: 1} } })
        };

        try {
            await IotData.updateThingShadow(params).promise();
            const speakOutput = 'La válvula se ha abierto. ¿Qué te gustaría hacer ahora? Puedes decir "cerrar la válvula" o "consultar el estado"';
            return handlerInput.responseBuilder
                .speak(speakOutput)
                .reprompt(speakOutput)
                .getResponse();
        } catch (err) {
            console.log(err);
            const speakOutput = 'Hubo un problema al abrir la válvula, por favor intenta nuevamente.';
            return handlerInput.responseBuilder
                .speak(speakOutput)
                .reprompt(speakOutput)
                .getResponse();
        }
    }
};


const HelpIntentHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest' &&
            Alexa.getIntentName(handlerInput.requestEnvelope) === 'AMAZON.HelpIntent';
    },
    handle(handlerInput) {
        const speakOutput = 'Puedes decir "abrir la válvula", "cerrar la válvula", "consultar el estado", "ajustar humedad recomendada" o "configurar dispositivo". ¿Cómo te puedo ayudar?';

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt(speakOutput)
            .getResponse();
    }
};

const CancelAndStopIntentHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest' &&
            (Alexa.getIntentName(handlerInput.requestEnvelope) === 'AMAZON.CancelIntent' ||
                Alexa.getIntentName(handlerInput.requestEnvelope) === 'AMAZON.StopIntent');
    },
    handle(handlerInput) {
        const speakOutput = '¡Adiós!';

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .getResponse();
    }
};

const SessionEndedRequestHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'SessionEndedRequest';
    },
    handle(handlerInput) {
        console.log(`Session ended with reason: ${handlerInput.requestEnvelope.request.reason}`);

        return handlerInput.responseBuilder.getResponse();
    }
};

const IntentReflectorHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest';
    },
    handle(handlerInput) {
        const intentName = Alexa.getIntentName(handlerInput.requestEnvelope);
        const speakOutput = `Acabas de invocar ${intentName}`;

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .getResponse();
    }
};

const ErrorHandler = {
    canHandle() {
        return true;
    },
    handle(handlerInput, error) {
        console.log(`Error handled: ${error.message}`);

        const speakOutput = 'Lo siento, tuve problemas para hacer lo que me pediste. Por favor intenta nuevamente.';

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt(speakOutput)
            .getResponse();
    }
};

exports.handler = Alexa.SkillBuilders.custom()
    .addRequestHandlers(
        LaunchRequestHandler,
        CaptureThingNameIntentHandler,
        CaptureAreaIntentHandler,
        CapturePlantTypeIntentHandler,
        StateIntentHandler,
        CloseValveIntentHandler,
        OpenValveIntentHandler,
        HelpIntentHandler,
        CancelAndStopIntentHandler,
        SessionEndedRequestHandler,
        IntentReflectorHandler
    )
    .addErrorHandlers(
        ErrorHandler
    )
    .lambda();
