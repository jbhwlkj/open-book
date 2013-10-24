/**
 * Copyright (c) 2012 Nokia Corporation.
 */

#include "xmle.h"
#include <stdio.h>

using namespace TXML;


/**
 * XMLElement for reading from stream with Qt
 *
 */


Node::Node(const char *name, const char *attr, const char *data) {
	m_data = 0;
	m_next = 0;
	m_children = 0;
	m_attributes = 0;

	setName(name);
	if (attr && attr[0]) applyAttributeString( attr );
	if (data && data[0]) {
		setData( data, strlen( data ) );
	}
}


Node::~Node() {
	setData(0,0);
	releaseAttributes();
	releaseChildren();
}

/**
 *
 * Read attributes to node from XML-formatted attribute string.
 *
 */
void Node::applyAttributeString( const char *string ) {
	char tebuffer[256];			// note.... will this be enough? long parameters may cause a crash
	int f;
	int p = 0;
	
	while (string[p]!=0) {
		while (string[p]!=0 && (string[p]==' ' || string[p]==10 || string[p]==13 || string[p]=='\t')) p++;
		if (string[p] == 0) break;
		f=0;
		while (string[p]!=' ' && string[p]!=0 && string[p]!='=') { tebuffer[f]=string[p]; f++; p++; }
		if (string[p]==0) break;			// not finished
		tebuffer[f] =0;
	
		SAttribute *natr = new SAttribute;
		natr->next = 0;
        natr->name.set( tebuffer );

		while (string[p]!=0 && string[p]!='"') p++;
		p++;
		f=0;
		while (string[p]!='"' && string[p]!=0) { tebuffer[f] = string[p]; f++; p++; }
		if (string[p]=='"') p++;
		tebuffer[f] = 0;
        natr->data.set( tebuffer );
		
		if (!m_attributes) 
			m_attributes = natr;
		else {
			SAttribute *l = m_attributes;
			while (l->next) l = l->next;
			l->next = natr;
        }
	} 
}


void Node::setData( const char *data, int length ) {
	if (m_data) delete [] m_data;
	if (data && length>0) {
		m_data = new char[ length + 1];
		memcpy( m_data, data, length );
		m_data[ length ]  = 0;
	} else m_data = 0;
}

Node* Node::addChild( Node *c ) {
	if (c==0) return 0;
	c->m_next = 0;
	if (m_children == 0) 
		m_children = c; 
	else {
		Node *l = m_children;
		while (l->m_next) l = l->m_next;
		l->m_next = c;
	}
	return c;
}

void Node::releaseChildren() {
	Node *l = m_children;
	while (l) {
		Node *n = l->m_next;
		delete l;
		l = n;
    }
	m_children = 0;
}

Node* Node::gotoPath( const char *path ) {
	int i;
	char *n;
	Node *l = m_children;
	while (l) {
		n = l->name();
		i=0;
		while (path[i]!=0 && path[i]!='/' && path[i]==n[i]) i++;
		if (path[i]==0) return l;
		if (path[i]=='/') return l->gotoPath( path+i+1 );
		l = l->next();
    }
	return 0;
}


void Node::releaseAttributes() {
	SAttribute *l = m_attributes;
	while (l) {
		SAttribute *n = l->next;
		delete l;
		l = n;
    }
	m_attributes = 0;
}


char *Node::getAttribute( char *name ) {
	SAttribute *l = m_attributes;
	while (l) {
        if (strcmp( name, l->name.getData() ) == 0) {
            return l->data.getData();
        }
		l = l->next;
    }
	return "";
}


int Node::getAttributeCount() {
	int rval = 0;
	SAttribute *l = m_attributes;
	while (l) {
		rval++;
		l = l->next;
    }
	return rval;
}


void Node::saveXML( QIODevice &target, int level ) {
	if (level==0) {
        target.write( "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" );
    }
    for (int tabs=0; tabs<level; tabs++) target.write( " " );
	char testr[256];
	
	sprintf(testr, "<%s", name());
    target.write( testr  );
	
	if (m_attributes) {
		SAttribute *l = m_attributes;
		while (l) {
            sprintf(testr, " %s=\"%s\"", l->name.getData(), l->data.getData() );
            target.write( testr );
			l = l->next;
        }
    }
	
	if (m_data || m_children) {
        target.write(">");
		
		if (m_children) {
            target.write("\n");
			Node *l = children();
			while (l) {
				l->saveXML( target, level +1 );
				l = l->next();
            }
            for (int tabs=0; tabs<level; tabs++) target.write( " " );
		} else if (m_data) {
            target.write( m_data );
        }
		
		sprintf( testr, "</%s>\n", name() );
        target.write( testr );
	} else {
        target.write( "/>\n" );
    }
}
		
		
		
		
/**
 * 
 * 
 * 	XML- FILE READER
 * 
 * 
 */


Reader::Reader(NODE_READY_CALLBACK_TYPE nodeReadyCallback, void *dataToNodeReady ) {
	m_nodeReadyCallback = nodeReadyCallback;
	m_dataToNodeReady = dataToNodeReady;
	m_tempBuffer = 0;
	m_tempBufferWritePos = 0;
	m_tempBufferLength = 0;
	m_lastTagBegin = 0;
	m_readBufferPos = 0;
	m_readBufferReaded = 0;
}


Reader::~Reader() {
	if (m_tempBuffer) delete [] m_tempBuffer;	
}

void Reader::resetTemp() {
	m_tempBufferWritePos = 0;
}

// readed with temporary 1024 byte's buffer since low-performance file operations on some systems. 
const char Reader::nextChar( QIODevice &stream ) {
	
	if (m_tempBufferWritePos>=m_tempBufferLength) {					// extend the temp buffer if required
		int copysize = m_tempBufferLength;
		if (m_tempBufferLength<1) m_tempBufferLength = 512; else m_tempBufferLength*=2;
		char *newTemp = new char[ m_tempBufferLength ];
		if (copysize>0) memcpy( newTemp, m_tempBuffer, copysize );
		if (m_tempBuffer) delete [] m_tempBuffer;
		m_tempBuffer = newTemp;
    }

	if (m_readBufferPos>=m_readBufferReaded) {
        m_readBufferReaded = stream.read( m_readBuffer, 1024 );
		if (m_readBufferReaded<1) return 0;				// finished
		m_readBufferPos = 0;
    }
	char c = m_readBuffer[ m_readBufferPos ];
	m_readBufferPos++;
	
	m_tempBuffer[ m_tempBufferWritePos ] = c;
	m_tempBufferWritePos++;
	
	return c;
}


int Reader::overWhiteSpace( char *data, int pos, int dlength ) {
	while (pos<dlength && (data[pos]==' ' || data[pos]==13 || data[pos]==10 || data[pos]=='\t')) pos++;
	return pos;
}

void Reader::setTag( Reader::STag &tag, char *data, int dlength ) {
	tag.closing_tag = false;
	tag.has_content = true;
	
	int p = overWhiteSpace(data, 0, dlength );
	if (data[p]=='/') {	tag.closing_tag=true; tag.has_content=false; p++; } 
	int namebegin = p;
	
	
	// at the beginning of the name
	while (p<dlength && data[p]!=' ' && data[p]!='>' && data[p]!='/') p++;
	// set the name 	
	tag.name.set( data+namebegin, p-namebegin );
	
	
	
	int endpos = dlength-1;
	while (endpos>0 && (data[endpos]==' ' || data[endpos]==10 || data[endpos]==13 || data[endpos]=='\t')) endpos--;
	if (endpos>0 && data[endpos]=='/') { tag.has_content = false; dlength = endpos; }
			
	p = overWhiteSpace(data, p, dlength );
	// the beginning of the attributes if any
	if (data[p]!='>' && data[p]!='/') {
		int attrbegin = p;
		while (p<dlength && data[p]!='>') p++;
		tag.attr.set( data+attrbegin, p-attrbegin );
    }
}

bool Reader::nextTag( QIODevice &stream, Reader::STag &target ) {
	char c;
	do {
		c = nextChar( stream );
		if (c==0) return false;
		if (c=='<') {		// tag begin.
			// mark begin
			m_lastTagBegin = m_tempBufferWritePos;
			
			do {
				c = nextChar(stream);
				if (c=='>') break;
			} while (c!=0);
			if (c==0) break;
			setTag( target, m_tempBuffer + m_lastTagBegin, m_tempBufferWritePos-m_lastTagBegin-1 );
			return true;
        }
		
	} while (1);	
	return false;
}





Node* Reader::scanXML( QIODevice &stream,  Node *myNode ) {
	STag tag;
	do {
		
		tag.name.emphty();
		tag.attr.emphty();
		if (!nextTag( stream, tag )) break;

				
        if (tag.name.getData() && tag.name.getData()[0]!='?') {
			if (tag.closing_tag == false) {				// begin tag found. 
				resetTemp();
				if (!myNode) {
                    myNode = new Node( tag.name.getData(), tag.attr.getData() );		// create my node
				} else {
                    Node *child = myNode->addChild( new Node( tag.name.getData(), tag.attr.getData() ) );
					if (tag.has_content) {
						scanXML( stream, child );					// scan content.
						if (child->children() == 0) {				// there was no content -> assume data
							child->setData( m_tempBuffer, m_lastTagBegin-1 );
                        }
                        /*
						if (m_nodeReadyCallback) {
							(m_nodeReadyCallback)( child, m_dataToNodeReady );
                        }
                        */
                    }

                    if (m_nodeReadyCallback)
                        (m_nodeReadyCallback)( child, m_dataToNodeReady );



				}
			} else {									// is a closing tag
                if (myNode && tag.name.getData() && strcmp( myNode->name(), tag.name.getData() ) == 0) {
					// all is ok, .. return 
					break;
				} else {
					// error in xml structure
					if (myNode)
                        printf("FATAL ERROR IN STRUCTURE!! close for %s with %s\n", myNode->name(), tag.name.getData());
					break;
                }
            }
		}
		
	} while (1);
    /*
    if (m_nodeReadyCallback) {
        (m_nodeReadyCallback)( myNode, m_dataToNodeReady );
    }
    */
	return myNode;
}
	

