/**
 * Copyright (c) 2012 Nokia Corporation.
 */

#ifndef __TXML_DEFINES__
#define __TXML_DEFINES__

#include <QtCore/QByteArray>
#include <QtCore/QIODevice>


namespace TXML {
    class TextData {
    public:
        TextData() {data =0;}
        ~TextData() { emphty(); }

        void emphty() {
            if (data) delete [] data;
            data = 0;
        }

        void set( const char *string ) {
            if (data) delete [] data;
            if (string==0 || string[0]==0) return;
            int len = strlen( string );
            data = new char[len+1];
            memcpy( data, string, len );
            data[len] = 0;
        }

        void set( const char *p, int length ) {
            if (data) delete [] data;
            data = new char[length+1];
            memcpy( data, p, length );
            data[length] = 0;
        }


        inline char *getData() { return data; }
    protected:
        char *data;
    };


	class Node {
	public:
		Node( const char *name, const char *attr = 0, const char *data = 0 );
        ~Node();
		
		
		Node *gotoPath( const char *path );
        void setName( const char *name ) { m_name.set(name); }
		
		
        inline char *name() { return (char*)m_name.getData(); }
		inline char *data() { return m_data; }
		void setData( const char *data, int length );
		
		inline Node* children() { return m_children; }
		inline Node* next() { return m_next; }
		
		void releaseChildren();			
		Node* addChild( Node *c );
		
		
		char *getAttribute( char *name );			// set attribute?
		int getAttributeCount();
		void applyAttributeString( const char *string );
		void releaseAttributes();
		
        void saveXML( QIODevice &target, int level = 0);
				
				
	protected:
		struct SAttribute {
            TextData name;
            TextData data;
			SAttribute *next;
		};
		
		SAttribute *m_attributes;
		
        TextData m_name;
		char *m_data;
		
		Node *m_next;
		Node *m_children;
	};



	typedef void (*NODE_READY_CALLBACK_TYPE)(Node *ready, void *userdata);
	
	class Reader {
	public:
		Reader( NODE_READY_CALLBACK_TYPE nodeReadyCallback = 0, void *dataToNodeReady = 0);
		~Reader();
        Node *scanXML( QIODevice &stream, Node *myNode = 0 );
		
		
	protected:
		NODE_READY_CALLBACK_TYPE m_nodeReadyCallback;
		void *m_dataToNodeReady;

		int overWhiteSpace( char *data, int pos, int dlength );
		struct STag {
            TextData name;
            TextData attr;
			bool closing_tag;
			bool has_content;
		};
		void setTag( STag &target, char *data, int datalength );
		
        bool nextTag( QIODevice &stream, STag &ntag );
		
		
		
		void resetTemp();
        const char nextChar( QIODevice &stream );
		
		char *m_tempBuffer;
		unsigned int m_tempBufferLength;
		unsigned int m_tempBufferWritePos;
		unsigned int m_lastTagBegin;
		
		
		// another temp for reading
		char m_readBuffer[1024];
		int m_readBufferPos;
		int m_readBufferReaded;
		
	};
}

#endif
