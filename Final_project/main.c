#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <ctype.h>

#define RESET_COLOR "\x1b[0m"
#define BRIGHT_BLACK "\x1b[90m"
#define BRIGHT_RED "\x1b[91m"
#define BRIGHT_GREEN "\x1b[92m"
#define BRIGHT_YELLOW "\x1b[93m"
#define BRIGHT_BLUE "\x1b[94m"
#define BRIGHT_MAGENTA "\x1b[95m"
#define BRIGHT_CYAN "\x1b[96m"
#define BRIGHT_WHITE "\x1b[97m"

#define MAX_QUESTIONS 100
#define MAX_NAME 50

typedef struct {
    char question[256];
    char options[4][100];
    int correctAnswer;
    bool used;
} Question;

typedef struct {
    char name[MAX_NAME];
    float score;
} Player;

Question questions[MAX_QUESTIONS];
int questionCount = 0;
Player leaderboard[100];
int leaderboardSize = 0;

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int getValidatedIntegerInput(const char* prompt, int min, int max) {
    int choice;
    char buffer[100];
    while (true) {
        printf("%s", prompt);
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            if (sscanf(buffer, "%d", &choice) == 1) {
                if (choice >= min && choice <= max) {
                    return choice;
                } else {
                    printf(BRIGHT_RED "Invalid choice! Please enter a number between %d and %d.\n" RESET_COLOR, min, max);
                }
            } else {
                printf(BRIGHT_RED "Invalid input! Please enter a number.\n" RESET_COLOR);
            }
        } else {
            printf(BRIGHT_RED "Error reading input. Please try again.\n" RESET_COLOR);
            clearInputBuffer();
        }
    }
}

char getValidatedCharAnswer(const char* prompt, bool fiftyUsed) {
    char inputChar;
    char buffer[100];
    while (true) {
        printf("%s", prompt);
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            inputChar = toupper(buffer[0]);

            if (strlen(buffer) == 2 && buffer[1] == '\n') {
                if ((inputChar >= 'A' && inputChar <= 'D') || (inputChar == 'E' && !fiftyUsed)) {
                    return inputChar;
                } else {
                    printf(BRIGHT_RED "Invalid input! Please enter A, B, C, D%s.\n" RESET_COLOR, fiftyUsed ? "" : ", or E");
                }
            } else {
                printf(BRIGHT_RED "Invalid input! Please enter only one character (A-D%s).\n" RESET_COLOR, fiftyUsed ? "" : ", or E");
            }
        } else {
            printf(BRIGHT_RED "Error reading input. Please try again.\n" RESET_COLOR);
            clearInputBuffer();
        }
    }
}

void loadQuestionsFromFile(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf(BRIGHT_RED "Could not open questions file. Using default questions.\n" RESET_COLOR);
        return;
    }

    questionCount = 0;
    char line[512];
    while (fgets(line, sizeof(line), file) && questionCount < MAX_QUESTIONS) {
        if (line[0] == '\n' || line[0] == '\0') continue;

        line[strcspn(line, "\n")] = '\0';

        char* token = strtok(line, ",");
        if (token) {
            strncpy(questions[questionCount].question, token, sizeof(questions[questionCount].question) - 1);
            questions[questionCount].question[sizeof(questions[questionCount].question) - 1] = '\0';
        } else continue;

        for (int i = 0; i < 4; i++) {
            token = strtok(NULL, ",");
            if (token) {
                snprintf(questions[questionCount].options[i], sizeof(questions[questionCount].options[i]), "%c. %s", 'A' + i, token);
            } else {
                questions[questionCount].question[0] = '\0';
                break;
            }
        }

        if (questions[questionCount].question[0] == '\0') continue;

        token = strtok(NULL, ",");
        if (token) {
            int correctIdx = atoi(token);
            if (correctIdx >= 0 && correctIdx <= 3) {
                questions[questionCount].correctAnswer = correctIdx;
            } else {
                printf(BRIGHT_YELLOW "Warning: Invalid correct answer index for question '%s'. Skipping.\n" RESET_COLOR, questions[questionCount].question);
                continue;
            }
        } else continue;

        questions[questionCount].used = false;
        questionCount++;
    }

    fclose(file);
}

void saveDefaultQuestions(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf(BRIGHT_RED "Could not create questions file '%s'.\n" RESET_COLOR, filename);
        return;
    }

    const char* defaultQuestions[] = {
        "What is the capital of Brazil?,Rio de Janeiro,Brasilia,Sao Paulo,Salvador,1",
        "Which planet is known as the Red Planet?,Venus,Mars,Jupiter,Saturn,1",
        "What is 2 + 2?,22,4,44,2,1",
        "Who painted the Mona Lisa?,Van Gogh,Picasso,Da Vinci,Monet,2",
        "What is the largest ocean?,Atlantic,Indian,Arctic,Pacific,3",
        "What is the chemical symbol for water?,Wa,H2O,O2,H,1",
        "How many continents are there?,5,6,7,8,2",
        "What is the smallest country in the world?,Monaco,Vatican City,Nauru,San Marino,1",
        "Which of these is a primary color?,Green,Orange,Purple,Yellow,3",
        "What year did the Titanic sink?,1912,1905,1923,1918,0"
    };

    int numDefaultQuestions = sizeof(defaultQuestions) / sizeof(defaultQuestions[0]);
    for (int i = 0; i < numDefaultQuestions; i++) {
        fprintf(file, "%s\n", defaultQuestions[i]);
    }

    fclose(file);
}

void resetQuestions() {
    for(int i = 0; i < questionCount; i++) {
        questions[i].used = false;
    }
}

int getRandomQuestionIndex() {
    int available[MAX_QUESTIONS];
    int count = 0;

    for(int i = 0; i < questionCount; i++) {
        if(!questions[i].used) {
            available[count++] = i;
        }
    }

    if(count == 0) return -1;
    return available[rand() % count];
}

void playGame() {
    float score = 0;
    int questionsAsked = 0;
    bool fiftyUsed = false;

    resetQuestions();
    clearScreen();
    printf(BRIGHT_YELLOW "=== Who Wants to Be a Millionaire ===\n\n" RESET_COLOR);

    while(questionsAsked < 15 && questionsAsked < questionCount) {
        int qIndex = getRandomQuestionIndex();
        if(qIndex == -1) {
            printf(BRIGHT_YELLOW "Not enough unique questions available to play. Please add more questions.\n" RESET_COLOR);
            break;
        }

        char playerAnswer;
        int answerIndex;
        bool answeredCorrectly = false;

        while(true) {
            clearScreen();
            printf(BRIGHT_YELLOW "Question %d:\n" RESET_COLOR, questionsAsked + 1);
            printf(BRIGHT_BLUE "%s\n" RESET_COLOR, questions[qIndex].question);

            for(int i = 0; i < 4; i++) {
                printf(BRIGHT_BLACK "%c. " BRIGHT_BLUE "%s\n" RESET_COLOR, 'A' + i, questions[qIndex].options[i] + 3);
            }

            playerAnswer = getValidatedCharAnswer(
                fiftyUsed ? BRIGHT_BLACK "\nEnter your answer " BRIGHT_BLUE "(A-D)" BRIGHT_BLACK ": " RESET_COLOR
                          : BRIGHT_BLACK "\nEnter your answer " BRIGHT_BLUE "(A-D)" BRIGHT_BLACK " or " BRIGHT_BLUE "E " BRIGHT_BLACK "for 50/50 lifeline: " RESET_COLOR,
                fiftyUsed
            );

            if (playerAnswer == 'E' && !fiftyUsed) {
                fiftyUsed = true;
                clearScreen();
                printf(BRIGHT_YELLOW "Question %d (50/50 Lifeline Used):\n" RESET_COLOR, questionsAsked + 1);
                printf(BRIGHT_BLUE "%s\n" RESET_COLOR, questions[qIndex].question);

                int correct = questions[qIndex].correctAnswer;
                int wrong1 = -1, wrong2 = -1;

                int eliminatedCount = 0;
                while (eliminatedCount < 2) {
                    int randomOption = rand() % 4;
                    if (randomOption != correct && randomOption != wrong1 && randomOption != wrong2) {
                        if (eliminatedCount == 0) wrong1 = randomOption;
                        else wrong2 = randomOption;
                        eliminatedCount++;
                    }
                }

                for (int i = 0; i < 4; i++) {
                    if (i == correct || (i != wrong1 && i != wrong2 && i != correct)) {
                        printf(BRIGHT_BLACK "%c. " BRIGHT_BLUE "%s\n" RESET_COLOR, 'A' + i, questions[qIndex].options[i] + 3);
                    }
                }
                printf(BRIGHT_YELLOW "\nLifeline used! Now choose from remaining options.\n" RESET_COLOR);
                printf("Press Enter to continue...");
                getchar();
                continue;
            } else {
                answerIndex = playerAnswer - 'A';
                if (answerIndex == questions[qIndex].correctAnswer) {
                    printf(BRIGHT_GREEN "\nCorrect!\n" RESET_COLOR);
                    score += 1.0;
                    questionsAsked++;
                    answeredCorrectly = true;
                } else {
                    printf(BRIGHT_RED "\nIncorrect! The correct answer was " BRIGHT_GREEN "%c. %s\n" RESET_COLOR,
                           'A' + questions[qIndex].correctAnswer, questions[qIndex].options[questions[qIndex].correctAnswer] + 3);
                    answeredCorrectly = false;
                }
                break;
            }
        }

        questions[qIndex].used = true;

        if (!answeredCorrectly) {
            printf(BRIGHT_YELLOW "Game Over!\n" RESET_COLOR);
            break;
        }

        printf("Press Enter to continue...");
        getchar();
    }

    clearScreen();
    printf(BRIGHT_CYAN "Game Over! Your final score: " BRIGHT_GREEN "%.1f\n" RESET_COLOR, score);

    char nameBuffer[MAX_NAME + 10];
    while (true) {
        printf(BRIGHT_BLACK "Enter your name " BRIGHT_BLUE "(max %d characters)" BRIGHT_BLACK ": " RESET_COLOR, MAX_NAME - 1);
        if (fgets(nameBuffer, sizeof(nameBuffer), stdin) != NULL) {
            nameBuffer[strcspn(nameBuffer, "\n")] = '\0';

            if (strlen(nameBuffer) > 0 && strlen(nameBuffer) < MAX_NAME) {
                break;
            } else {
                printf(BRIGHT_RED "Invalid name! Name cannot be empty and must be less than %d characters.\n" RESET_COLOR, MAX_NAME);
            }
        } else {
            printf(BRIGHT_RED "Error reading name. Please try again.\n" RESET_COLOR);
            clearInputBuffer();
        }
    }

    if (leaderboardSize < 100) {
        strcpy(leaderboard[leaderboardSize].name, nameBuffer);
        leaderboard[leaderboardSize].score = score;
        leaderboardSize++;
        printf(BRIGHT_GREEN "Score added to leaderboard!\n" RESET_COLOR);
    } else {
        printf(BRIGHT_YELLOW "Leaderboard is full. Score not saved.\n" RESET_COLOR);
    }

    printf("\nPress Enter to return to main menu...");
    getchar();
}

void showLeaderboard() {
    clearScreen();
    printf(BRIGHT_YELLOW "=== Leaderboard ===\n\n" RESET_COLOR);
    if (leaderboardSize == 0) {
        printf(BRIGHT_CYAN "No scores recorded yet. Play a game to add entries!\n" RESET_COLOR);
    } else {
        printf(BRIGHT_BLACK "No. " BRIGHT_BLUE "Player Name" BRIGHT_BLACK " \t " BRIGHT_GREEN "Score\n" RESET_COLOR);
        printf(BRIGHT_BLACK "-----------------------------------\n" RESET_COLOR);
        for(int i = 0; i < leaderboardSize; i++) {
            printf(BRIGHT_BLACK "%-3d. " BRIGHT_BLUE "%-15s" BRIGHT_BLACK "\t " BRIGHT_GREEN "%.1f\n" RESET_COLOR,
                   i + 1, leaderboard[i].name, leaderboard[i].score);
        }
    }
    printf("\nPress Enter to return to main menu...");
    getchar();
}

void showPlayerHistory() {
    clearScreen();
    printf(BRIGHT_YELLOW "=== Player History ===\n\n" RESET_COLOR);
    char nameToSearch[MAX_NAME];
    char buffer[MAX_NAME + 10];

    while (true) {
        printf(BRIGHT_BLACK "Enter player name to search " BRIGHT_BLUE "(max %d characters)" BRIGHT_BLACK ": " RESET_COLOR, MAX_NAME - 1);
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            buffer[strcspn(buffer, "\n")] = '\0';

            if (strlen(buffer) > 0 && strlen(buffer) < MAX_NAME) {
                strcpy(nameToSearch, buffer);
                break;
            } else {
                printf(BRIGHT_RED "Invalid name! Name cannot be empty and must be less than %d characters.\n" RESET_COLOR, MAX_NAME);
            }
        } else {
            printf(BRIGHT_RED "Error reading name. Please try again.\n" RESET_COLOR);
            clearInputBuffer();
        }
    }

    clearScreen();
    printf(BRIGHT_CYAN "History for " BRIGHT_BLUE "%s" BRIGHT_CYAN ":\n" RESET_COLOR, nameToSearch);
    bool found = false;
    for(int i = 0; i < leaderboardSize; i++) {
        if(strcmp(leaderboard[i].name, nameToSearch) == 0) {
            printf(BRIGHT_BLACK " - Score: " BRIGHT_GREEN "%.1f\n" RESET_COLOR, leaderboard[i].score);
            found = true;
        }
    }
    if(!found) printf(BRIGHT_CYAN "No history found for " BRIGHT_BLUE "%s" BRIGHT_CYAN ".\n" RESET_COLOR, nameToSearch);

    printf("\nPress Enter to return to main menu...");
    getchar();
}

void mainMenu() {
    int choice;
    do {
        clearScreen();
        printf(BRIGHT_YELLOW "=== Who Wants to Be a Millionaire ===\n" RESET_COLOR);
        printf(BRIGHT_MAGENTA "1. " BRIGHT_GREEN "Play Game\n" RESET_COLOR);
        printf(BRIGHT_MAGENTA "2. " BRIGHT_YELLOW "View Leaderboard\n" RESET_COLOR);
        printf(BRIGHT_MAGENTA "3. " BRIGHT_CYAN "View Player History\n" RESET_COLOR);
        printf(BRIGHT_MAGENTA "4. " BRIGHT_RED "Exit\n" RESET_COLOR);
        printf(BRIGHT_BLACK "-----------------------------------\n" RESET_COLOR);

        choice = getValidatedIntegerInput(BRIGHT_BLACK "Enter your choice " BRIGHT_BLUE "(1-4)" BRIGHT_BLACK ": " RESET_COLOR, 1, 4);

        switch(choice) {
            case 1:
                playGame();
                break;
            case 2:
                showLeaderboard();
                break;
            case 3:
                showPlayerHistory();
                break;
            case 4:
                clearScreen();
                printf(BRIGHT_YELLOW "Goodbye! Thanks for playing!\n" RESET_COLOR);
                break;
            default:
                printf(BRIGHT_RED "Invalid choice! Please try again.\n" RESET_COLOR);
                getchar();
                break;
        }
    } while(choice != 4);
}

int main() {
    srand(time(NULL));

    const char* filename = "questions.txt";
    FILE* fileCheck = fopen(filename, "r");
    if (fileCheck == NULL) {
        saveDefaultQuestions(filename);
    } else {
        fclose(fileCheck);
    }

    loadQuestionsFromFile(filename);

    mainMenu();

    return 0;
}
