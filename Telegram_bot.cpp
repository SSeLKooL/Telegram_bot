#include <stdio.h>
#include <tgbot/tgbot.h>

using namespace TgBot;
using namespace std;

string const defaultRecipeMenuText = "Выберите понравившийся рецепт";
string const DBDir = "DB/", TextDir = DBDir + "Text/";
string const EndTag = "!End", ImageTag = "!Img";
int const StackSize = 9;

int FindSpaceIndex(string s, int from)
{
  while (from < s.length() && s[from] != '_')
  {
    from++;
  }

  return from;
}

struct StackData
{
  int id;
  string fileName;
  int dir = 0;

  StackData(string data)
  {
    int from = 0;
    int to = FindSpaceIndex(data, from);
    dir = stoi(data.substr(from, to - from));
    from = to + 1;
    to = FindSpaceIndex(data, from);
    fileName = data.substr(from, to - from);
    from = to + 1;
    to = FindSpaceIndex(data, from);
    id = stoi(data.substr(from, to - from));
    id += dir;
  }

  StackData(int _id, string _fileName)
  {
    id = _id;
    fileName = _fileName;
  }
};

void tryRead(ifstream &in, string &line)
{
  getline(in, line);
  if (in.eof())
  {
    throw 1;
  }
}

struct Recipe
{
  string header;
  string Ingredients = "";
  string cookingMethod = "";

  Recipe(StackData sd)
  {
    ifstream in;
    string line;
    in.open(TextDir + sd.fileName);
    if (!in)
    {
      throw 0;
    }
    int cnt = 0, endcnt = 2;

    while (cnt != sd.id)
    {
      tryRead(in, line);

      if (StringTools::startsWith(line, "name: "))
      {
        if (endcnt < 2)
        {
          throw 2;
        }
        else if (endcnt > 2)
        {
          throw 3;
        }
        endcnt = 0;
        cnt++;
      }
      else if (line == EndTag)
      {
        endcnt++;
      }
    }

    while (true)
    {
      tryRead(in, line);

      if (StringTools::startsWith(line, "name: "))
      {
        if (endcnt < 2)
        {
          throw 2;
        }
        else if (endcnt > 2)
        {
          throw 3;
        }
        endcnt = 0;
        break;
      }
      else if (line == EndTag)
      {
        endcnt++;
      }
    }

    string name = line.substr(6);
    string portions;
    tryRead(in, portions);

    header = name + '\n' + portions + '\n';

    tryRead(in, line);
    while (line != EndTag)
    {
      Ingredients += line + '\n';
      tryRead(in, line);
      if (StringTools::startsWith(line, "name: "))
      {
        throw 2;
      }
    }

    tryRead(in, line);
    while (line != EndTag)
    {
      cookingMethod += line + '\n';
      tryRead(in, line);
      if (StringTools::startsWith(line, "name: "))
      {
        throw 2;
      }
    }

    in.close();
  }

  void SendRecipe(Bot &bot, int64_t chatId)
  {
    string recipeText = header + Ingredients + cookingMethod;
    int imageStart = 0, prev = 0;

    while ((imageStart = recipeText.find(ImageTag, imageStart)) != -1)
    {
      int imageEnd = recipeText.find("\n", imageStart);
      string imageLink = recipeText.substr(imageStart + ImageTag.length(), imageEnd - imageStart - ImageTag.length());
      bot.getApi().sendMessage(chatId, recipeText.substr(prev, imageStart - prev));
      bot.getApi().sendPhoto(chatId, imageLink);
      imageStart = imageEnd + 1;
      prev = imageStart;
    }
    bot.getApi().sendMessage(chatId, recipeText.substr(prev, recipeText.length() - prev));
  }
};

vector<string> GetStack(string fileName, int id)
{
  vector<string> st;
  ifstream in;
  in.open(TextDir + fileName);
  if (!in)
  {
    throw 0;
  }
  int cnt = 0;
  int curId = 0;

  string line;

  while (curId != id)
  {
    tryRead(in, line);
    if (StringTools::startsWith(line, "name: "))
    {
      cnt++;
      if (cnt == StackSize)
      {
        cnt = 0;
        curId++;
      }
    }
  }

  while (cnt != StackSize)
  {
    getline(in, line);
    if (in.eof())
    {
      break;
    }

    if (StringTools::startsWith(line, "name: "))
    {
      cnt++;
      st.push_back(line.substr(6));
    }
  }

  in.close();

  return st;
}

int GetMaxStackId(string fileName)
{
  ifstream in;
  in.open(TextDir + fileName);
  if (!in)
  {
    throw 0;
  }

  int cnt = 0;

  string line;

  while (true)
  {
    getline(in, line);
    if (in.eof())
    {
      break;
    }

    if (StringTools::startsWith(line, "name: "))
    {
      cnt++;
    }
  }

  in.close();
  return ceil((long double)cnt / StackSize) - 1;
}

void createInlineKeyboard(StackData sd, InlineKeyboardMarkup::Ptr& kb)
{
  kb->inlineKeyboard.clear();
  vector<string> buttonLayout = GetStack(sd.fileName, sd.id);

  int const lineSize = 3;
  int ColumnSize = ceil((long double)buttonLayout.size() / lineSize);

  for (int i = 0; i < ColumnSize; i++)
  {
    int cnt = min(lineSize, (int)buttonLayout.size() - i * lineSize);
    vector<InlineKeyboardButton::Ptr> row;

    for (int j = 0; j < cnt; j++)
    {
      InlineKeyboardButton::Ptr button(new InlineKeyboardButton);
      button->text = buttonLayout[i * lineSize + j];
      button->callbackData = "0_" + sd.fileName + '_' + to_string(sd.id * StackSize + i * lineSize + j);
      row.push_back(button);
    }

    kb->inlineKeyboard.push_back(row);
  }

  vector<InlineKeyboardButton::Ptr> lastRow;

  if (sd.id != 0)
  {
    InlineKeyboardButton::Ptr button(new InlineKeyboardButton);
    button->text = "Назад";
    button->callbackData = "-1_" + sd.fileName + '_' + to_string(sd.id);
    lastRow.push_back(button);
  }

  if (sd.id != GetMaxStackId(sd.fileName))
  {
    InlineKeyboardButton::Ptr button(new InlineKeyboardButton);
    button->text = "Вперед";
    button->callbackData = "1_" + sd.fileName + '_' + to_string(sd.id);
    lastRow.push_back(button);
  }

  kb->inlineKeyboard.push_back(lastRow);
}

void createKeyboard(const vector<vector<pair<string, string>>>& buttonLayout, ReplyKeyboardMarkup::Ptr& kb)
{
  for (auto line : buttonLayout)
  {
    vector<KeyboardButton::Ptr> row;

    for (auto block : line)
    {
      KeyboardButton::Ptr button(new KeyboardButton);
      button->text = block.first;
      row.push_back(button);
    }

    kb->keyboard.push_back(row);
  }
}

void GetErrorInfo(int errorCode)
{
  cout << "ERROR with code " << errorCode << endl;
  switch (errorCode)
  {
    case 0:
      cout << "Can't open file" << endl;
      break;
    case 1:
      cout << "DataBase format error: Unexpected end of file" << endl;
      break;
    case 2:
      cout << "DataBase format error: Can't find end tag" << endl;
      break;
    case 3:
      cout << "DataBase format error: Can't find name tag" << endl;
      break;
  }
}

int main()
{
    Bot bot("HERE YOUR TOKEN");

    vector<vector<pair<string, string>>> FoodTypes =
    {
      {{"Горячее", "Hot.txt"}, {"Салаты", "Salad.txt"}},
      {{"Закуски", "Snacks.txt"}, {"Супы", "Soups.txt"}},
      {{"Выпечка", "Bakery.txt"}, {"Десерты", "Dessert.txt"}},
      {{"Напитки", "Beverages.txt"}, {"Соусы", "Sauces.txt"}}
    };

    ReplyKeyboardMarkup::Ptr mainKeyBoard(new ReplyKeyboardMarkup);
    createKeyboard(FoodTypes, mainKeyBoard);

    InlineKeyboardMarkup::Ptr messageKeyBoard(new InlineKeyboardMarkup);

    bot.getEvents().onCommand("start", [&bot, &mainKeyBoard](Message::Ptr message)
    {
      bot.getApi().sendMessage(message->chat->id, "Привет, " + message->chat->firstName, false, 0, mainKeyBoard);
    });

    bot.getEvents().onAnyMessage([&bot, &FoodTypes, &messageKeyBoard](Message::Ptr message)
    {
      try
      {
        if (message->text == "/start")
        {
          return ;
        }

        for (auto line : FoodTypes)
        {
          for (auto block : line)
          {
            if (message->text == block.first)
            {
              StackData cur(0, block.second);
              createInlineKeyboard(cur, messageKeyBoard);
              bot.getApi().sendMessage(message->chat->id, defaultRecipeMenuText, false, 0, messageKeyBoard);
              return ;
            }
          }
        }

        bot.getApi().sendMessage(message->chat->id, "Я глупенький и не понимаю. Используйте меню снизу");
      }
      catch(int errorCode)
      {
        GetErrorInfo(errorCode);
      }
    });

    bot.getEvents().onCallbackQuery([&bot, &messageKeyBoard](CallbackQuery::Ptr query)
    {
      try
      {
        bot.getApi().getUpdates();
        StackData cur(query->data);

        if (cur.dir == 0)
        {
          Recipe recipe(cur);
          recipe.SendRecipe(bot, query->message->chat->id);
        }
        else
        {
          createInlineKeyboard(cur, messageKeyBoard);
          bot.getApi().editMessageText(query->message->text + "...", query->message->chat->id, query->message->messageId, "", "", 0, messageKeyBoard);
          bot.getApi().editMessageText(defaultRecipeMenuText, query->message->chat->id, query->message->messageId, "", "", 0, messageKeyBoard);
        }
      }
      catch(int errorCode)
      {
        GetErrorInfo(errorCode);
      }
    });

    cout << "Bot username: " << bot.getApi().getMe()->username.c_str() << '\n';
    try
    {
      TgLongPoll longPoll(bot);
      while (true)
      {
          cout << "Long poll started\n";
          longPoll.start();
      }

    }
    catch (TgException& e)
    {
      cout << "error: " << e.what() << '\n';
    }

    return 0;
}
