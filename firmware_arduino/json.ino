
#define JSON_MAX_DEPTH 255
byte json_depth;          // how many levels of recursion in the object structure
bool json_first;          //set true when entering into a new collection so that a comma is not added

/* starts the json file */
void json_start(String &dst)
{
  dst = "{";
  json_depth = 0;
  json_first = true;
}

/* call to move down through the heirachy */
bool json_down()
{
  json_depth++;
  if(json_depth < JSON_MAX_DEPTH)
  {
    json_first = true;
    return true;
  }
  else 
  {
    json_depth--;        return false;
  }
}

/* call to move up the heirachy, to close a collection */
void json_up()
{
  if(json_depth) json_depth--;
  else {} //end of json file in this case
}


/* new line, add tabs to depth*/
void json_nl(String &dst)
{
  char tabs[json_depth+1];
  memset(tabs,'\t',json_depth);
  dst += "\n";
  dst += tabs;
}

/* append a string in quotes from sram */
void json_string(String &dst, const char *label_pgm)
{
  dst += "\"";
  dst += label_pgm;
  dst += "\"";
}
/* append a string in quotes from progmem */
void json_string(String &dst, const __FlashStringHelper *label_pgm)
{
  dst += "\"";
  dst += label_pgm;
  dst += "\"";
}

/* add a new entry to the current collection */
void json_entry(String &dst)
{
  if(!json_first)  {     dst += ",";   } //add list separator if not first
  else {    json_first = false;  }
}


/* add an element to an object, should be followed by a value (number, string, array or object) */
void json_label(String &dst, const char *label_pgm)
{
  json_string(dst, label_pgm);
  dst += ":";
}
void json_label(String &dst, const __FlashStringHelper *label_pgm)
{
  json_string(dst, label_pgm);
  dst += ":";
}

/* start a new object, follow with {json_entry, opt. new line, label, value} (number, string, array or object) */
void json_object(String &dst)
{
  dst += "{";
  json_first = true;
  json_down();
}
/* end the current object */
void json_close_object(String &dst)
{
  dst += "}";
  json_up();
}

/* start a new array, follow with {json_entry, opt. new line, value} (number, string, array or object)*/
void json_array(String &dst)
{
  dst += "[";
  json_first = true;
  json_down();
}
/* end the current array */
void json_close_array(String &dst)
{
  dst += "]";
  json_up();
}

/* add an integer value */
void json_int(String &dst, int value)
{
  dst += value;
}

/* add an array and populate with numbers */
void json_int_array(String &dst, int *values, int count)
{
  json_array(dst);
  while(count--)
  {
    json_entry(dst);
    json_int(dst,*values++);
  }
  json_close_array(dst);
}

/* add an entry on a new line with the given label */
void json_obj_entry(String &dst, const char* label)
{
  json_entry(dst);
  json_nl(dst);
  json_label(dst, label);
}
/* as above, label in prgmem */
void json_obj_entry(String &dst, const __FlashStringHelper * label)
{
  json_entry(dst);
  json_nl(dst);
  json_label(dst, label);
}

/* add an entry to the current object with the given label, for ints and strings with either label or sting in progmem */
void json_obj(String &dst, const char* label, const char* value) //string
{
  json_obj_entry(dst, label);
  json_string(dst, value);
}
void json_obj(String &dst, const char* label, const __FlashStringHelper* value) //_string
{
  json_obj_entry(dst, label);
  json_string(dst, value);
}
void json_obj(String &dst, const __FlashStringHelper* label, const char* value) //_string
{
  json_obj_entry(dst, label);
  json_string(dst, value);
}
void json_obj(String &dst, const __FlashStringHelper* label, const __FlashStringHelper* value) //_string
{
  json_obj_entry(dst, label);
  json_string(dst, value);
}
void json_obj(String &dst, const char* label, int value) //_int
{
  json_obj_entry(dst, label);
  json_int(dst, value);
}
void json_obj(String &dst, const __FlashStringHelper* label, int value) //_int
{
  json_obj_entry(dst, label);
  json_int(dst, value);
}
