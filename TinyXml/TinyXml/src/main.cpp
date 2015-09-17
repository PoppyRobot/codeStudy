#include <string>
#include <iostream>
#include <tinyxml.h>
#include <iconv.h>

#include "testObjectClass.h"

#pragma comment(lib, "libIconv.lib")

void printSchoolXml();
void readSchoolXml();
void writeSchoolXml();

/* 转码函数 */
int code_convert(char *from_charset, char *to_charset, const char *inbuf, size_t inlen, char *outbuf, size_t outlen)
{
	iconv_t cd;
	const char **pin = &inbuf;
	char **pout = &outbuf;

	cd = iconv_open(to_charset, from_charset);
	if ((iconv_t)-1 == cd) 
		return -1;
	memset(outbuf, 0, outlen);
	if (-1 == iconv(cd, pin, &inlen,pout, &outlen)) 
		return -1;
	iconv_close(cd);

	return 0;
}

/* UTF-8 to GBK  */
int u2g(const char *inbuf, size_t inlen, char *outbuf, size_t outlen)
{
	return code_convert("UTF-8", "GBK", inbuf, inlen, outbuf, outlen);
}

int main(int argc, char** argv) {
	//printSchoolXml();
	readSchoolXml();
	//writeSchoolXml();

	test();

	return 0;
}

void printSchoolXml() {
	using namespace std;
	TiXmlDocument doc;  
	const char * xmlFile = "conf/school.xml";	
	if (doc.LoadFile(xmlFile)) {  	
		doc.Print();  
	} else {
		cout << "can not parse xml conf/school.xml" << endl;
	}
}

void readSchoolXml() 
{
	using namespace std;

	const char *xmlFile = "conf/school.xml";

	TiXmlDocument doc;
	if (!doc.LoadFile(xmlFile))
	{
		cout << "Load xml file failed.\t" << xmlFile << endl;
		return;
	}

	cout << "Load xml file OK." << endl;
	
	TiXmlDeclaration *decl = doc.FirstChild()->ToDeclaration();

	if (!decl)
	{
		cout << "decl is null.\n" << endl;
		return;
	}
	
	cout <<  decl->Encoding();
	cout <<  decl->Version();
	cout <<  decl->Standalone() << endl;



	TiXmlHandle docHandle(&doc);
	TiXmlElement *child 
		= docHandle.FirstChild("School").FirstChild("Class").Child("Student", 0).ToElement();

	for ( ; child != NULL; child = child->NextSiblingElement())
	{
		TiXmlAttribute *attr = child->FirstAttribute();

		for (; attr != NULL; attr = attr->Next())
		{
			cout << attr->Name() << " : " << attr->Value() << endl;
		}
		
		TiXmlElement *ct = child->FirstChildElement();
		for (; ct != NULL; ct = ct->NextSiblingElement())
		{
			char buf[1024] = {0};
			u2g(ct->GetText(), strlen(ct->GetText()), buf, sizeof(buf));
			cout << ct->Value() << " : " << buf << endl;
		}
		cout << "=====================================" << endl;
	}
	
	TiXmlElement *schl = doc.RootElement();
	const char *value_t =schl->Attribute("name");
	
	char buf[1024] = {0};
	if ( u2g(value_t, strlen(value_t), buf, sizeof(buf)) == -1) {
		return;
	}
	cout << "Root Element value: " << buf << endl;
	schl->RemoveAttribute("name");
	schl->SetValue("NewSchool");


	cout << "Save file: " << (doc.SaveFile("conf/new.xml") ? "Ok" : "Failed") << endl;

return ;
	TiXmlElement *rootElement = doc.RootElement();
	TiXmlElement *classElement = rootElement->FirstChildElement();
	TiXmlElement *studentElement = classElement->FirstChildElement();


	// N 个 Student 节点
	for ( ; studentElement!= NULL; studentElement = studentElement->NextSiblingElement()) 
	{
		// 获得student
		TiXmlAttribute *stuAttribute = studentElement->FirstAttribute();
		for (; stuAttribute != NULL; stuAttribute = stuAttribute->Next()) 
		{
			cout << stuAttribute->Name() << " : " << stuAttribute->Value() << endl;
		}

		// 获得student的第一个联系方式 
		TiXmlElement *contact = studentElement->FirstChildElement();
		for (; contact != NULL; contact = contact->NextSiblingElement())
		{
			const char *text = contact->GetText();
			char buf[1024] = {0};
			if ( u2g(text, strlen(text), buf, sizeof(buf)) == -1) {
				continue;
			}
			cout << contact->Value() << " : " << buf << endl; 
		}
	}
}


void writeSchoolXml() {
	using namespace std;
	const char * xmlFile = "conf/school-write.xml";	
	TiXmlDocument doc;  
	TiXmlDeclaration * decl = new TiXmlDeclaration("1.0", "", "");  
	TiXmlElement * schoolElement = new TiXmlElement( "School" );  
	TiXmlElement * classElement = new TiXmlElement( "Class" );  
	classElement->SetAttribute("name", "C++");

	TiXmlElement * stu1Element = new TiXmlElement("Student");
	stu1Element->SetAttribute("name", "tinyxml");
	stu1Element->SetAttribute("number", "123");
	TiXmlElement * stu1EmailElement = new TiXmlElement("email");
	stu1EmailElement->LinkEndChild(new TiXmlText("tinyxml@163.com") );
	TiXmlElement * stu1AddressElement = new TiXmlElement("address");
	stu1AddressElement->LinkEndChild(new TiXmlText("中国"));
	stu1Element->LinkEndChild(stu1EmailElement);
	stu1Element->LinkEndChild(stu1AddressElement);

	TiXmlElement * stu2Element = new TiXmlElement("Student");
	stu2Element->SetAttribute("name", "jsoncpp");
	stu2Element->SetAttribute("number", "456");
	TiXmlElement * stu2EmailElement = new TiXmlElement("email");
	stu2EmailElement->LinkEndChild(new TiXmlText("jsoncpp@163.com"));
	TiXmlElement * stu2AddressElement = new TiXmlElement("address");
	stu2AddressElement->LinkEndChild(new TiXmlText("美国"));
	stu2Element->LinkEndChild(stu2EmailElement);
	stu2Element->LinkEndChild(stu2AddressElement);

	classElement->LinkEndChild(stu1Element);  
	classElement->LinkEndChild(stu2Element);  
	schoolElement->LinkEndChild(classElement);  
	
	doc.LinkEndChild(decl);  
	doc.LinkEndChild(schoolElement); 
	doc.SaveFile(xmlFile);  
}
