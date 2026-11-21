#include "types.h"
#include "app/research_radar/research_data.h"

static u8 ResearchData_GetQuestionIndex(const ResearchData *data, u8 questionId);

u16 ResearchData_GetAnswerID(const ResearchData *data, u8 question, u8 answer) {
    return data->questions[question].answers[answer].id;
}

u8 ResearchData_GetAnswerRed(const ResearchData *data, u8 question, u8 answer) {
    return data->questions[question].answers[answer].red;
}

u8 ResearchData_GetAnswerGreen(const ResearchData *data, u8 question, u8 answer) {
    return data->questions[question].answers[answer].green;
}

u8 ResearchData_GetAnswerBlue(const ResearchData *data, u8 question, u8 answer) {
    return data->questions[question].answers[answer].blue;
}

// The index of a question by its ID, or 0 if it isn't there
static u8 ResearchData_GetQuestionIndex(const ResearchData *data, u8 questionId) {
    int i;

    for (i = 0; i < RESEARCH_QUESTION_COUNT; i++) {
        if (questionId == data->questions[i].id) {
            return i;
        }
    }
    return 0;
}

u8 ResearchData_GetAnswerIndex(const ResearchData *data, u8 questionId, u8 answerId) {
    int i;
    const ResearchQuestion *question = &data->questions[ResearchData_GetQuestionIndex(data, questionId)];

    for (i = 0; i < question->answerCount; i++) {
        if (answerId == question->answers[i].id) {
            return i;
        }
    }
    return 0;
}
