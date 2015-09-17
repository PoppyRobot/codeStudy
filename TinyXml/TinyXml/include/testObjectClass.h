#ifndef TESTOBJECTCLASS_H_
#define TESTOBJECTCLASS_H_

#include <string>
#include <map>
#include <list>

using namespace std;

typedef std::map<string, string> MessageMap;

class WindowSettings
{
public:
	int x, y, w, h;
	string name;

	WindowSettings() : x(0), y(0), w(100), h(100), name("Untitiled")
	{

	}

	WindowSettings(int x, int y, int w, int h, string name)
	{
		this->x = x;
		this->y = y;
		this->w = w;
		this->h = h;
		this->name = name;
	}
};

class ConnectSettings
{
public:
	string ip;
	double time_out;

	ConnectSettings() : ip("127.0.0.1"), time_out(123.456)
	{

	}

	ConnectSettings(string &ip, double time_out)
	{
		this->ip = ip;
		this->time_out = time_out;
	}
};

class AppSettings
{
public:
	string     m_name;
	MessageMap m_messages;
	list<WindowSettings> m_windows;
	ConnectSettings m_conection;

	AppSettings () {}

	void setDemoValues()
	{
		m_name = "MyApp";
		m_messages.clear();
		m_messages["Welcome"] = "Wecome Tiny XML";
		m_messages["Farewell"] = "Thank you for using";
		m_messages["Welcome2"] = "Wecome Tiny XML 2";
		m_messages["Farewell2"] = "Thank you for using 2";
		
		m_windows.push_back(WindowSettings(10, 10, 200, 200, "InitWindow"));
		m_windows.push_back(WindowSettings(10, 10, 200, 200, "SecondWindow"));

		m_conection.ip = "192.168.1.1";
		m_conection.time_out = 125.321;
	}

	void save(const char *filename)
	{
		TiXmlDocument doc;
		TiXmlElement  *msg;
		TiXmlComment  *comment;
		string s;

		TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "", "");
		doc.LinkEndChild(decl);

		TiXmlElement *root = new TiXmlElement(m_name.c_str());
		doc.LinkEndChild(root);

		comment = new TiXmlComment();
		s = " Setting for " + m_name + " ";
		comment->SetValue(s.c_str());
		root->LinkEndChild(comment);

		// block Message
		{
			MessageMap::iterator iter;
			TiXmlElement *msgs = new TiXmlElement("Messages");
			root->LinkEndChild(msgs);

			for (iter = m_messages.begin(); iter != m_messages.end(); iter++)
			{
				const string &key = (*iter).first;
				const string &value = (*iter).second;
				msg = new TiXmlElement(key.c_str());
				msg->LinkEndChild(new TiXmlText(value.c_str()));
				msgs->LinkEndChild(msg);

				/*if (key == "Welcome")
				{
					TiXmlElement *new_ele = new TiXmlElement("SecondWelcome");
					msg->LinkEndChild(new_ele);
					new_ele->SetAttribute("Name", "NewName");
					new_ele->SetAttribute("IP", "192.168.100.110");
				}*/
			}
		}

		//windows
		{
			TiXmlElement *windowNode = new TiXmlElement("Windows");
			root->LinkEndChild(windowNode);
			while (!m_windows.empty())
			{
				const WindowSettings& st = m_windows.front();
				
				TiXmlElement *window = new TiXmlElement("Window");
				windowNode->LinkEndChild(window);
				window->SetAttribute("name", st.name.c_str());
				window->SetAttribute("x", st.x);
				window->SetAttribute("y", st.y);
				window->SetAttribute("w", st.w);
				window->SetAttribute("h", st.h);
				m_windows.pop_front();
			}
		}

		//Connect Settings
		{
			TiXmlElement *connect = new TiXmlElement("Connection");
			root->LinkEndChild(connect);

			connect->SetAttribute("ip", m_conection.ip.c_str());
			connect->SetDoubleAttribute("timeout", m_conection.time_out);
		}

		cout << "Save File: " << filename << (doc.SaveFile(filename) ? " OK" : " Failed") << endl;
		doc.Clear();
	}

	void load(const char *filename)
	{
		TiXmlDocument doc(filename);
		if (!doc.LoadFile())
		{
			return;
		}

		TiXmlHandle hDoc(&doc);
		TiXmlElement *pElem;
		TiXmlHandle hRoot(0);

		// block: name
		{
			pElem = hDoc.FirstChildElement().Element();
			if (!pElem)
			{
				return;
			}

			m_name = pElem->Value();
			hRoot = TiXmlHandle(pElem);
		}

		// block: string table
		{
			m_messages.clear();
			pElem = hRoot.FirstChild("Messages").FirstChild().Element();
			for (pElem; pElem; pElem = pElem->NextSiblingElement())
			{
				const char *pKey = pElem->Value();
				const char *pText = pElem->GetText();
				if (pKey && pText)
				{
					m_messages[pKey] = pText;
				}
			}
		}
	}
};

void test()
{
	AppSettings app;
	const char *filename = "conf/demo.xml";

	app.setDemoValues();

	app.save(filename);

	app.load(filename);
}

#endif
