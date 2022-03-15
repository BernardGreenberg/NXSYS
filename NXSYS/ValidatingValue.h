//
//  ValidatingValue.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 3/14/22.
//  Copyright © 2022 BernardGreenberg. All rights reserved.
//

#ifndef ValidatingValue_h
#define ValidatingValue_h

/* Uso típico
 
 using std::string;
 ValidatingValue<string> GetReason (item){
    .....
    if (seguro)
        return (string)"Hay siete enanos para la chica.";
    else return {};
 }
 
 ....
 if (auto r = GetReason (thingy))
     cout << (string)r << endl;
 
 */

template <class C>
class ValidatingValue {
public:
  C value;
  bool valid;

  operator C () {return value;}
  operator bool () {return valid;}

  ValidatingValue () : valid(false), value{} {};
  ValidatingValue (C v) : value(v), valid(true){};
};


#endif /* ValidatingValue_h */
