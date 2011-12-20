/*
Copyright (c) 2011 Elektrobit (EB), All rights reserved.
Contact: oss-devel@elektrobit.com

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of the Elektrobit (EB) nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY Elektrobit (EB) ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Elektrobit
(EB) BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

package eu.licentia.necessitas.mobile;
//@ANDROID-5
//QtCreator import java.text.DateFormat;
//QtCreator import java.text.ParseException;
//QtCreator import java.text.SimpleDateFormat;
//QtCreator import java.util.Calendar;
//QtCreator import java.util.Date;
//QtCreator 
//QtCreator import android.content.ContentResolver;
//QtCreator import android.content.ContentUris;
//QtCreator import android.content.ContentValues;
//QtCreator import android.database.Cursor;
//QtCreator import android.net.Uri;
//QtCreator import android.provider.ContactsContract;
//QtCreator import android.provider.ContactsContract.CommonDataKinds.Email;
//QtCreator import android.provider.ContactsContract.CommonDataKinds.Event;
//QtCreator import android.provider.ContactsContract.CommonDataKinds.Nickname;
//QtCreator import android.provider.ContactsContract.CommonDataKinds.Note;
//QtCreator import android.provider.ContactsContract.CommonDataKinds.Organization;
//QtCreator import android.provider.ContactsContract.CommonDataKinds.Phone;
//QtCreator import android.provider.ContactsContract.CommonDataKinds.StructuredName;
//QtCreator import android.provider.ContactsContract.CommonDataKinds.StructuredPostal;
//QtCreator import android.provider.ContactsContract.CommonDataKinds.Website;
//QtCreator import android.provider.ContactsContract.Contacts.Data;
//QtCreator import android.provider.ContactsContract.RawContacts;
//QtCreator import android.util.Log;
//QtCreator import eu.licentia.necessitas.industrius.QtApplication;
//QtCreator public class QtAndroidContacts
//QtCreator {
//QtCreator     private static AndroidContacts m_androidContacts;
//QtCreator 
//QtCreator     @SuppressWarnings("unused")
//QtCreator     private void getContacts()
//QtCreator     {
//QtCreator         Cursor c=QtApplication.mainActivity().getContentResolver().query(ContactsContract.Contacts.CONTENT_URI,null, null, null, null);
//QtCreator         m_androidContacts = new AndroidContacts(c.getCount());
//QtCreator         for(int i=0;i<c.getCount();i++)
//QtCreator         {
//QtCreator             c.moveToNext();
//QtCreator             String id = c.getString(c.getColumnIndex(ContactsContract.Contacts._ID));
//QtCreator             String displayName = c.getString(c.getColumnIndex(ContactsContract.Contacts.DISPLAY_NAME));
//QtCreator             NameData nameData = getNameDetails(id);
//QtCreator             PhoneNumber[] numberArray = {};
//QtCreator             if (Integer.parseInt(c.getString(c.getColumnIndex(ContactsContract.Contacts.HAS_PHONE_NUMBER))) > 0)
//QtCreator             {
//QtCreator                     numberArray = getPhoneNumberDetails(id);
//QtCreator             }
//QtCreator             EmailData[] emailArray = getEmailAddressDetails(id);
//QtCreator             String note = getNoteDetails(id);
//QtCreator             AddressData[] addressArray= getAddressDetails(id);
//QtCreator             OrganizationalData[] organizationArray = getOrganizationDetails(id);
//QtCreator             String birthday = getBirthdayDetails(id);
//QtCreator             String[] urlArray =  getUrlDetails(id);
//QtCreator             String anniversary = getAnniversaryDetails(id);
//QtCreator             String nickName = getNickNameDetails(id);
//QtCreator             OnlineAccount[] onlineAccountArray = getImDetails(id);
//QtCreator             m_androidContacts.buildContacts(i,id,displayName,nameData,numberArray,emailArray,note,addressArray,organizationArray,birthday,anniversary,nickName,urlArray,onlineAccountArray);
//QtCreator         }
//QtCreator     }
//QtCreator 
//QtCreator     private NameData getNameDetails(String id)
//QtCreator     {
//QtCreator         String nameWhere = ContactsContract.Data.CONTACT_ID + " = ? AND " + ContactsContract.Data.MIMETYPE + " = ?";
//QtCreator         String[] nameWhereParams = new String[]{id,
//QtCreator                         ContactsContract.CommonDataKinds.StructuredName.CONTENT_ITEM_TYPE};
//QtCreator         Cursor nameCur = QtApplication.mainActivity().getContentResolver().query(ContactsContract.Data.CONTENT_URI,
//QtCreator                         null, nameWhere, nameWhereParams, null);
//QtCreator         if(nameCur != null && nameCur.moveToFirst())
//QtCreator         {
//QtCreator             String firstName = nameCur.getString(nameCur.getColumnIndex(ContactsContract.CommonDataKinds.StructuredName.GIVEN_NAME));
//QtCreator             if(firstName == null)
//QtCreator                     firstName = "";
//QtCreator             String lastName = nameCur.getString(nameCur.getColumnIndex(ContactsContract.CommonDataKinds.StructuredName.FAMILY_NAME));
//QtCreator             if(lastName == null)
//QtCreator                     lastName = "";
//QtCreator             String middleName = nameCur.getString(nameCur.getColumnIndex(ContactsContract.CommonDataKinds.StructuredName.MIDDLE_NAME));
//QtCreator             if(middleName == null)
//QtCreator                     middleName = "";
//QtCreator             String prefix  = nameCur.getString(nameCur.getColumnIndex(ContactsContract.CommonDataKinds.StructuredName.PREFIX));
//QtCreator             if(prefix == null)
//QtCreator                     prefix = "";
//QtCreator             String suffix = nameCur.getString(nameCur.getColumnIndex(ContactsContract.CommonDataKinds.StructuredName.SUFFIX));
//QtCreator             if(suffix == null)
//QtCreator                     suffix = "";
//QtCreator             NameData nameDetails = new NameData(firstName,lastName,middleName,prefix,suffix);
//QtCreator             nameCur.close();
//QtCreator             return nameDetails;
//QtCreator         }
//QtCreator         return null;
//QtCreator     }
//QtCreator 
//QtCreator     private PhoneNumber[] getPhoneNumberDetails(String id)
//QtCreator     {
//QtCreator         Cursor pCur = QtApplication.mainActivity().getContentResolver().query(
//QtCreator                         ContactsContract.CommonDataKinds.Phone.CONTENT_URI,
//QtCreator                         null,
//QtCreator                         ContactsContract.CommonDataKinds.Phone.CONTACT_ID +" = ?",
//QtCreator                         new String[]{id}, null);
//QtCreator         if(pCur!= null)
//QtCreator         {
//QtCreator             int i=0;
//QtCreator             PhoneNumber[] numbers = new PhoneNumber[pCur.getCount()];
//QtCreator             while (pCur.moveToNext())
//QtCreator             {
//QtCreator                 String number= pCur.getString(pCur.getColumnIndex(ContactsContract.CommonDataKinds.Phone.NUMBER));
//QtCreator                 int type =Integer.parseInt(pCur.getString(pCur.getColumnIndex(ContactsContract.CommonDataKinds.Phone.TYPE)));
//QtCreator                 numbers[i] = new PhoneNumber(number, type);
//QtCreator                 i++;
//QtCreator             }
//QtCreator             pCur.close();
//QtCreator             return numbers;
//QtCreator         }
//QtCreator         return null;
//QtCreator     }
//QtCreator 
//QtCreator     private EmailData[] getEmailAddressDetails(String id)
//QtCreator     {
//QtCreator         Cursor emailCur = QtApplication.mainActivity().getContentResolver().query(
//QtCreator                         ContactsContract.CommonDataKinds.Email.CONTENT_URI,
//QtCreator                         null,
//QtCreator                         ContactsContract.CommonDataKinds.Email.CONTACT_ID + " = ?",
//QtCreator                         new String[]{id}, null);
//QtCreator         if(emailCur!= null)
//QtCreator         {
//QtCreator             EmailData[] emails = new EmailData[emailCur.getCount()];
//QtCreator             int i=0;
//QtCreator             while (emailCur.moveToNext())
//QtCreator             {
//QtCreator                     String email = emailCur.getString(emailCur.getColumnIndex(ContactsContract.CommonDataKinds.Email.DATA));
//QtCreator                     String type = emailCur.getString(emailCur.getColumnIndex(ContactsContract.CommonDataKinds.Email.TYPE));
//QtCreator                     emails[i] = new EmailData(email, Integer.parseInt(type));
//QtCreator                     i++;
//QtCreator             }
//QtCreator             emailCur.close();
//QtCreator             return emails;
//QtCreator         }
//QtCreator         return null;
//QtCreator     }
//QtCreator 
//QtCreator     private String getNoteDetails(String id)
//QtCreator     {
//QtCreator         String noteWhere = ContactsContract.Data.CONTACT_ID + " = ? AND " + ContactsContract.Data.MIMETYPE + " = ?";
//QtCreator         String[] noteWhereParams = new String[]{id, ContactsContract.CommonDataKinds.Note.CONTENT_ITEM_TYPE};
//QtCreator         Cursor noteCur = QtApplication.mainActivity().getContentResolver().query(ContactsContract.Data.CONTENT_URI, null, noteWhere, noteWhereParams, null);
//QtCreator         String note = null;
//QtCreator         if (noteCur.moveToFirst()) {
//QtCreator             note = noteCur.getString(noteCur.getColumnIndex(ContactsContract.CommonDataKinds.Note.NOTE));
//QtCreator         }
//QtCreator         noteCur.close();
//QtCreator         return note;
//QtCreator     }
//QtCreator 
//QtCreator     private String getBirthdayDetails(String id)
//QtCreator     {
//QtCreator         Cursor birthDayCur =QtApplication.mainActivity().getContentResolver().query( ContactsContract.Data.CONTENT_URI, new String[] { Event.DATA }, ContactsContract.Data.CONTACT_ID + "=" + id + " AND " + Data.MIMETYPE + "= '" + Event.CONTENT_ITEM_TYPE + "' AND " + Event.TYPE + "=" + Event.TYPE_BIRTHDAY, null, null);
//QtCreator         if(birthDayCur.moveToFirst())
//QtCreator         {
//QtCreator             DateFormat df = DateFormat.getDateInstance(DateFormat.LONG);
//QtCreator             String birthday = null;
//QtCreator             try {
//QtCreator                 Date mydate = df.parse( birthDayCur.getString(0));
//QtCreator                 SimpleDateFormat newformat = new SimpleDateFormat("dd:MM:yyyy");
//QtCreator                 birthday = newformat.format(mydate);
//QtCreator             } catch (ParseException e) {
//QtCreator                 e.printStackTrace();
//QtCreator             }
//QtCreator             birthDayCur.close();
//QtCreator             return birthday;
//QtCreator         }
//QtCreator         return null;
//QtCreator     }
//QtCreator 
//QtCreator     private String getAnniversaryDetails(String id)
//QtCreator     {
//QtCreator         Cursor anniversaryCur =QtApplication.mainActivity().getContentResolver().query( ContactsContract.Data.CONTENT_URI, new String[] { Event.DATA }, ContactsContract.Data.CONTACT_ID + "=" + id + " AND " + Data.MIMETYPE + "= '" + Event.CONTENT_ITEM_TYPE + "' AND " + Event.TYPE + "=" + Event.TYPE_ANNIVERSARY, null, ContactsContract.Data.DISPLAY_NAME);
//QtCreator         if(anniversaryCur.moveToFirst())
//QtCreator         {
//QtCreator             DateFormat df = DateFormat.getDateInstance(DateFormat.LONG);
//QtCreator             String anniversary = null;
//QtCreator             try {
//QtCreator                 Date mydate = df.parse( anniversaryCur.getString(0));
//QtCreator                 SimpleDateFormat newformat = new SimpleDateFormat("dd:MM:yyyy");
//QtCreator                 anniversary = newformat.format(mydate);
//QtCreator             } catch (ParseException e) {
//QtCreator                 e.printStackTrace();
//QtCreator             }
//QtCreator             anniversaryCur.close();
//QtCreator             return anniversary;
//QtCreator         }
//QtCreator         return null;
//QtCreator     }
//QtCreator 
//QtCreator     private String getNickNameDetails(String id)
//QtCreator     {
//QtCreator         String nickWhere = ContactsContract.Data.CONTACT_ID + " = ? AND " + ContactsContract.Data.MIMETYPE + " = ?";
//QtCreator         String[] nickWhereParams = new String[]{id,
//QtCreator                         ContactsContract.CommonDataKinds.Nickname.CONTENT_ITEM_TYPE};
//QtCreator         Cursor nickCur = QtApplication.mainActivity().getContentResolver().query(ContactsContract.Data.CONTENT_URI,
//QtCreator                         null, nickWhere, nickWhereParams, null);
//QtCreator         if (nickCur.moveToFirst()) {
//QtCreator             String nickName= nickCur.getString(nickCur.getColumnIndex(ContactsContract.CommonDataKinds.Nickname.DATA));
//QtCreator             nickCur.close();
//QtCreator             return nickName;
//QtCreator         }
//QtCreator         return null;
//QtCreator     }
//QtCreator 
//QtCreator     private OrganizationalData[] getOrganizationDetails(String id)
//QtCreator     {
//QtCreator         String orgWhere = ContactsContract.Data.CONTACT_ID + " = ? AND " + ContactsContract.Data.MIMETYPE + " = ?";
//QtCreator         String[] orgWhereParams = new String[]{id,
//QtCreator                         ContactsContract.CommonDataKinds.Organization.CONTENT_ITEM_TYPE};
//QtCreator         Cursor orgCur = QtApplication.mainActivity().getContentResolver().query(ContactsContract.Data.CONTENT_URI,
//QtCreator                         null, orgWhere, orgWhereParams, null);
//QtCreator         if(orgCur!= null)
//QtCreator         {
//QtCreator             OrganizationalData[] organizations = new OrganizationalData[orgCur.getCount()];
//QtCreator             int i=0;
//QtCreator             while (orgCur.moveToNext())
//QtCreator             {
//QtCreator                 String organization = orgCur.getString(orgCur.getColumnIndex(ContactsContract.CommonDataKinds.Organization.DATA));
//QtCreator                 if(organization == null)
//QtCreator                         organization = "";
//QtCreator                 String title = orgCur.getString(orgCur.getColumnIndex(ContactsContract.CommonDataKinds.Organization.TITLE));
//QtCreator                 if(title == null)
//QtCreator                         title = "";
//QtCreator                 String type = orgCur.getString(orgCur.getColumnIndex(ContactsContract.CommonDataKinds.Organization.TYPE));
//QtCreator                 organizations[i]= new OrganizationalData(organization, title, Integer.parseInt(type));
//QtCreator                 i++;
//QtCreator             }
//QtCreator             orgCur.close();
//QtCreator             return organizations;
//QtCreator         }
//QtCreator         return null;
//QtCreator     }
//QtCreator 
//QtCreator     private AddressData[] getAddressDetails(String id)
//QtCreator     {
//QtCreator         String addrWhere = ContactsContract.Data.CONTACT_ID + " = ? AND " + ContactsContract.Data.MIMETYPE + " = ?";
//QtCreator         String[] addrWhereParams = new String[]{id,
//QtCreator                         ContactsContract.CommonDataKinds.StructuredPostal.CONTENT_ITEM_TYPE};
//QtCreator         Cursor addrCur = QtApplication.mainActivity().getContentResolver().query(ContactsContract.Data.CONTENT_URI,
//QtCreator                         null, addrWhere,addrWhereParams, null);
//QtCreator         if(addrCur != null)
//QtCreator         {
//QtCreator             AddressData[] addresses = new AddressData[addrCur.getCount()];
//QtCreator             int i=0;
//QtCreator             while(addrCur.moveToNext()) {
//QtCreator                 String pobox = addrCur.getString(addrCur.getColumnIndex(ContactsContract.CommonDataKinds.StructuredPostal.POBOX));
//QtCreator                 if(pobox == null)
//QtCreator                         pobox = "";
//QtCreator                 String street =addrCur.getString(addrCur.getColumnIndex(ContactsContract.CommonDataKinds.StructuredPostal.STREET));
//QtCreator                 if(street == null)
//QtCreator                         street = "";
//QtCreator                 String city =addrCur.getString(addrCur.getColumnIndex(ContactsContract.CommonDataKinds.StructuredPostal.CITY));
//QtCreator                 if(city == null)
//QtCreator                         city = "";
//QtCreator                 String region = addrCur.getString(addrCur.getColumnIndex(ContactsContract.CommonDataKinds.StructuredPostal.REGION));
//QtCreator                 if(region == null)
//QtCreator                         region = "";
//QtCreator                 String postCode = addrCur.getString(addrCur.getColumnIndex(ContactsContract.CommonDataKinds.StructuredPostal.POSTCODE));
//QtCreator                 if(postCode == null)
//QtCreator                         postCode = "";
//QtCreator                 String country = addrCur.getString(addrCur.getColumnIndex(ContactsContract.CommonDataKinds.StructuredPostal.COUNTRY));
//QtCreator                 if(country == null)
//QtCreator                         country = "";
//QtCreator                 String type = addrCur.getString(addrCur.getColumnIndex(ContactsContract.CommonDataKinds.StructuredPostal.TYPE));
//QtCreator                 addresses[i] = new AddressData(pobox, street, city, region, postCode, country, Integer.parseInt(type));
//QtCreator                 i++;
//QtCreator             }
//QtCreator             addrCur.close();
//QtCreator             return addresses;
//QtCreator         }
//QtCreator         return null;
//QtCreator     }
//QtCreator 
//QtCreator     private String[] getUrlDetails(String id)
//QtCreator     {
//QtCreator         String urlWhere = ContactsContract.Data.CONTACT_ID + " = ? AND " + ContactsContract.Data.MIMETYPE + " = ?";
//QtCreator         String[] urlWhereParams = new String[]{id, ContactsContract.CommonDataKinds.Website.CONTENT_ITEM_TYPE};
//QtCreator         Cursor urlCur = QtApplication.mainActivity().getContentResolver().query(ContactsContract.Data.CONTENT_URI, null, urlWhere, urlWhereParams, null);
//QtCreator         if(urlCur!= null)
//QtCreator         {
//QtCreator             String[] urls = new String[urlCur.getCount()];
//QtCreator             int i=0;
//QtCreator             while (urlCur.moveToNext())
//QtCreator             {
//QtCreator                 urls[i] =urlCur.getString(urlCur.getColumnIndex(ContactsContract.CommonDataKinds.Website.DATA));
//QtCreator                 i++;
//QtCreator             }
//QtCreator             urlCur.close();
//QtCreator             return urls;
//QtCreator         }
//QtCreator         return null;
//QtCreator     }
//QtCreator 
//QtCreator     private OnlineAccount[] getImDetails(String id)
//QtCreator     {
//QtCreator         String imWhere = ContactsContract.Data.CONTACT_ID + " = ? AND " + ContactsContract.Data.MIMETYPE + " = ?";
//QtCreator         String[] imWhereParams = new String[]{id, ContactsContract.CommonDataKinds.Im.CONTENT_ITEM_TYPE};
//QtCreator         Cursor imCur = QtApplication.mainActivity().getContentResolver().query(ContactsContract.Data.CONTENT_URI, null, imWhere, imWhereParams, null);
//QtCreator         if(imCur!=null)
//QtCreator         {
//QtCreator             OnlineAccount[] onlineAcoountArray = new OnlineAccount[imCur.getCount()];
//QtCreator             int i=0;
//QtCreator             while(imCur.moveToNext())
//QtCreator             {
//QtCreator                 String accountName = imCur.getString(imCur.getColumnIndex(ContactsContract.CommonDataKinds.Im.DATA));
//QtCreator                 if(accountName == null)
//QtCreator                 {
//QtCreator                         accountName = "";
//QtCreator                 }
//QtCreator                 int protocol=0;
//QtCreator                 String sProtocol = imCur.getString(imCur.getColumnIndex(ContactsContract.CommonDataKinds.Im.PROTOCOL));
//QtCreator                 if(sProtocol!=null)
//QtCreator                 {
//QtCreator                     protocol = Integer.parseInt(sProtocol);
//QtCreator                 }
//QtCreator                 long timeStamp=0;
//QtCreator                 String sTimeStamp=imCur.getString(imCur.getColumnIndex(ContactsContract.CommonDataKinds.Im.STATUS_TIMESTAMP));
//QtCreator                 if(sTimeStamp!=null)
//QtCreator                 {
//QtCreator                     timeStamp = Long.parseLong(sTimeStamp);
//QtCreator                 }
//QtCreator                 int presence=0;
//QtCreator                 String sPresence=imCur.getString(imCur.getColumnIndex(ContactsContract.CommonDataKinds.Im.PRESENCE));
//QtCreator                 if(sPresence!=null)
//QtCreator                 {
//QtCreator                     presence = Integer.parseInt(sPresence);
//QtCreator                 }
//QtCreator                 String status = imCur.getString(imCur.getColumnIndex(ContactsContract.CommonDataKinds.Im.STATUS));
//QtCreator                 if(status == null)
//QtCreator                 {
//QtCreator                     status = "";
//QtCreator                 }
//QtCreator                 int type = Integer.parseInt(imCur.getString(imCur.getColumnIndex(ContactsContract.CommonDataKinds.Im.TYPE)));
//QtCreator                 String customProtocol = null;
//QtCreator                 if(protocol == -1)
//QtCreator                 {
//QtCreator                     customProtocol = imCur.getString(imCur.getColumnIndex(ContactsContract.CommonDataKinds.Im.CUSTOM_PROTOCOL));
//QtCreator                     onlineAcoountArray[i] = new OnlineAccount(accountName, status, timeStamp,customProtocol, presence, protocol, type);
//QtCreator                 }
//QtCreator                 else
//QtCreator                 {
//QtCreator                     onlineAcoountArray[i] = new OnlineAccount(accountName, status, timeStamp,presence, protocol, type);
//QtCreator                 }
//QtCreator                 i++;
//QtCreator             }
//QtCreator             imCur.close();
//QtCreator             return onlineAcoountArray;
//QtCreator         }
//QtCreator         return null;
//QtCreator     }
//QtCreator 
//QtCreator     private String searchByRID(String rid)
//QtCreator     {
//QtCreator         String id=null;
//QtCreator         Cursor allContactsCur =  QtApplication.mainActivity().managedQuery(RawContacts.CONTENT_URI, new String[] {RawContacts._ID}, RawContacts.CONTACT_ID + " = " + rid, null, null);
//QtCreator         if(allContactsCur.moveToFirst())
//QtCreator         {
//QtCreator             id = allContactsCur.getString(allContactsCur.getColumnIndex(ContactsContract.Contacts._ID));
//QtCreator         }
//QtCreator         allContactsCur.close();
//QtCreator         return id;
//QtCreator     }
//QtCreator 
//QtCreator     @SuppressWarnings("unused")
//QtCreator     private String saveContact(MyContacts qtContacts)
//QtCreator     {
//QtCreator         ContentValues values = new ContentValues();
//QtCreator         String group = null;
//QtCreator         String id;
//QtCreator         values.put(RawContacts.ACCOUNT_TYPE,group);
//QtCreator         values.put(RawContacts.ACCOUNT_NAME, group);
//QtCreator         Uri rawContactUri = QtApplication.mainActivity().getContentResolver().insert(RawContacts.CONTENT_URI, values);
//QtCreator         long rawContactId = ContentUris.parseId(rawContactUri);
//QtCreator         saveNameDetails(qtContacts.m_names,values,rawContactId);
//QtCreator 
//QtCreator         int numberCount;
//QtCreator         numberCount = qtContacts.m_phoneNumbers.length;
//QtCreator         for(int i=0;i<numberCount;i++)
//QtCreator         {
//QtCreator             if(qtContacts.m_phoneNumbers[i].m_number.length()!= 0)
//QtCreator             {
//QtCreator                 String number = qtContacts.m_phoneNumbers[i].m_number;
//QtCreator                 int type = qtContacts.m_phoneNumbers[i].m_type;
//QtCreator                 savePhoneNumberDetails(number, type, values, rawContactId);
//QtCreator             }
//QtCreator         }
//QtCreator 
//QtCreator         int addrCount = qtContacts.m_address.length;
//QtCreator         for(int i=0;i<addrCount;i++)
//QtCreator         {
//QtCreator             if(qtContacts.m_address[i].m_pobox.length()!= 0
//QtCreator                             || qtContacts.m_address[i].m_street.length()!=0
//QtCreator                             || qtContacts.m_address[i].m_city.length()!=0
//QtCreator                             || qtContacts.m_address[i].m_region.length()!=0
//QtCreator                             || qtContacts.m_address[i].m_postCode.length()!=0
//QtCreator                             || qtContacts.m_address[i].m_country.length()!=0)
//QtCreator             {
//QtCreator                 saveAddressDetails(qtContacts.m_address[i].m_pobox,
//QtCreator                                 qtContacts.m_address[i].m_street,qtContacts.m_address[i].m_city,
//QtCreator                                 qtContacts.m_address[i].m_region, qtContacts.m_address[i].m_postCode,
//QtCreator                                 qtContacts.m_address[i].m_country,qtContacts.m_address[i].m_type,
//QtCreator                                 values, rawContactId);
//QtCreator             }
//QtCreator         }
//QtCreator 
//QtCreator         int orgCount = qtContacts.m_organizations.length;
//QtCreator         for(int i=0;i<orgCount;i++)
//QtCreator         {
//QtCreator             if(qtContacts.m_organizations[i].m_organization.length()!= 0
//QtCreator                     ||qtContacts.m_organizations[i].m_title.length()== 0)
//QtCreator             {
//QtCreator                 saveOrganizationalDetails(qtContacts.m_organizations[i].m_organization, qtContacts.m_organizations[i].m_title,
//QtCreator                                 qtContacts.m_organizations[i].m_type, values, rawContactId);
//QtCreator             }
//QtCreator         }
//QtCreator 
//QtCreator         int urlCount = qtContacts.m_contactUrls.length;
//QtCreator         for(int i=0;i<urlCount;i++)
//QtCreator         {
//QtCreator             if(qtContacts.m_contactUrls[i].length()!=0)
//QtCreator             {
//QtCreator                 saveUrlDetails(qtContacts.m_contactUrls[i], values, rawContactId);
//QtCreator             }
//QtCreator         }
//QtCreator 
//QtCreator         int emailCount = qtContacts.m_email.length;
//QtCreator         for(int i=0;i<emailCount;i++)
//QtCreator         {
//QtCreator             if(qtContacts.m_email[i].m_email.length()!=0)
//QtCreator             {
//QtCreator                 SaveEmailDetails(qtContacts.m_email[i].m_email, qtContacts.m_email[i].m_type,
//QtCreator                                 values, rawContactId);
//QtCreator             }
//QtCreator         }
//QtCreator 
//QtCreator         if(qtContacts.m_contactNote.length()!=0)
//QtCreator         {
//QtCreator             saveNoteDetails(qtContacts.m_contactNote, values, rawContactId);
//QtCreator         }
//QtCreator         if(qtContacts.m_contactNickName.length()!=0)
//QtCreator         {
//QtCreator             saveNickNameDetails(qtContacts.m_contactNickName, values, rawContactId);
//QtCreator         }
//QtCreator         if(qtContacts.m_contactBirthDay.length()!=0)
//QtCreator         {
//QtCreator             String[] birthdayInfo = qtContacts.m_contactBirthDay.split(":");
//QtCreator             int year = Integer.valueOf(birthdayInfo[0]);
//QtCreator             int month = Integer.parseInt(birthdayInfo[1], 10);
//QtCreator             month--;
//QtCreator             int day = Integer.parseInt(birthdayInfo[2],10);
//QtCreator             saveBirthdayDetails(year, month, day, values, rawContactId);
//QtCreator         }
//QtCreator         if(qtContacts.m_contactAnniversary.length()!=0)
//QtCreator         {
//QtCreator             String[] anniversaryInfo = qtContacts.m_contactAnniversary.split(":");
//QtCreator             int year = Integer.valueOf(anniversaryInfo[0]);
//QtCreator             int month = Integer.valueOf(anniversaryInfo[1]);
//QtCreator             month--;
//QtCreator             int day = Integer.parseInt(anniversaryInfo[2],10);
//QtCreator             saveAnniversaryDetails(year, month, day, values, rawContactId);
//QtCreator         }
//QtCreator         if(qtContacts.m_names.m_firstName.length()!=0)
//QtCreator         {
//QtCreator             id=searchByRID(Long.toString(rawContactId));
//QtCreator             return id;
//QtCreator         }
//QtCreator         return "";
//QtCreator     }
//QtCreator 
//QtCreator     private void saveNameDetails(NameData names,ContentValues values,long rawContactId)
//QtCreator     {
//QtCreator         values.clear();
//QtCreator         values.put(Data.RAW_CONTACT_ID, rawContactId);
//QtCreator         values.put(Data.MIMETYPE, StructuredName.CONTENT_ITEM_TYPE);
//QtCreator         if(names.m_firstName.length()!=0)
//QtCreator         {
//QtCreator             values.put(StructuredName.GIVEN_NAME,names.m_firstName);
//QtCreator         }
//QtCreator         if(names.m_lastName.length()!=0)
//QtCreator         {
//QtCreator             values.put(StructuredName.FAMILY_NAME,names.m_lastName);
//QtCreator         }
//QtCreator         if(names.m_middleName.length()!=0)
//QtCreator         {
//QtCreator             values.put(StructuredName.MIDDLE_NAME,names.m_middleName);
//QtCreator         }
//QtCreator         if(names.m_prefix.length()!=0)
//QtCreator         {
//QtCreator             values.put(StructuredName.PREFIX,names.m_prefix);
//QtCreator         }
//QtCreator         if(names.m_suffix.length()!=0)
//QtCreator         {
//QtCreator             values.put(StructuredName.SUFFIX,names.m_suffix);
//QtCreator         }
//QtCreator         QtApplication.mainActivity().getContentResolver().insert( ContactsContract.Data.CONTENT_URI, values);
//QtCreator     }
//QtCreator 
//QtCreator     private void savePhoneNumberDetails(String number,int type,ContentValues values,long rawContactId)
//QtCreator     {
//QtCreator         values.clear();
//QtCreator         values.put(Data.RAW_CONTACT_ID, rawContactId);
//QtCreator         values.put(Data.MIMETYPE,Phone.CONTENT_ITEM_TYPE);
//QtCreator         values.put(Phone.NUMBER,number);
//QtCreator         values.put(Phone.TYPE, type);
//QtCreator         QtApplication.mainActivity().getContentResolver().insert(ContactsContract.Data.CONTENT_URI, values);
//QtCreator     }
//QtCreator 
//QtCreator     private void saveAddressDetails(String postbox,String Street,String city,String region,String postcode,String country,int type,ContentValues values,long rawContactId)
//QtCreator     {
//QtCreator         values.clear();
//QtCreator         values.put(Data.RAW_CONTACT_ID, rawContactId);
//QtCreator         values.put(Data.MIMETYPE,StructuredPostal.CONTENT_ITEM_TYPE);
//QtCreator         if(postbox.length()!=0)
//QtCreator                 values.put(StructuredPostal.POBOX, postbox);
//QtCreator         if(Street.length()!=0)
//QtCreator                 values.put(StructuredPostal.STREET, Street);
//QtCreator         if(city.length()!=0)
//QtCreator                 values.put(StructuredPostal.CITY,city);
//QtCreator         if(region.length()!=0)
//QtCreator                 values.put(StructuredPostal.REGION,region);
//QtCreator         if(postcode.length()!=0)
//QtCreator                 values.put(StructuredPostal.POSTCODE,postcode);
//QtCreator         if(country.length()!=0)
//QtCreator                 values.put(StructuredPostal.COUNTRY,country);
//QtCreator         values.put(StructuredPostal.TYPE, type);
//QtCreator         QtApplication.mainActivity().getContentResolver().insert(ContactsContract.Data.CONTENT_URI, values);
//QtCreator     }
//QtCreator 
//QtCreator     private void saveOrganizationalDetails(String orgname,String title,int type,ContentValues values,long rawContactId)
//QtCreator     {
//QtCreator         values.clear();
//QtCreator         values.put(Data.RAW_CONTACT_ID, rawContactId);
//QtCreator         values.put(Data.MIMETYPE,Organization.CONTENT_ITEM_TYPE);
//QtCreator         if(orgname.length()!= 0)
//QtCreator                 values.put(Organization.COMPANY, orgname);
//QtCreator         if(title.length()!= 0)
//QtCreator                 values.put(Organization.TITLE,title);
//QtCreator         values.put(Organization.TYPE,type);
//QtCreator         QtApplication.mainActivity().getContentResolver().insert(ContactsContract.Data.CONTENT_URI, values);
//QtCreator     }
//QtCreator     private void saveUrlDetails(String url,ContentValues values,long rawContactId)
//QtCreator     {
//QtCreator         values.clear();
//QtCreator         values.put(Data.RAW_CONTACT_ID, rawContactId);
//QtCreator         values.put(Data.MIMETYPE,Website.CONTENT_ITEM_TYPE);
//QtCreator         values.put(Website.URL, url);
//QtCreator         values.put(Website.TYPE,Website.TYPE_HOMEPAGE);
//QtCreator         QtApplication.mainActivity().getContentResolver().insert(ContactsContract.Data.CONTENT_URI,values);
//QtCreator     }
//QtCreator 
//QtCreator     private void SaveEmailDetails(String email,int type,ContentValues values,long rawContactId)
//QtCreator     {
//QtCreator         values.clear();
//QtCreator         values.put(Data.RAW_CONTACT_ID, rawContactId);
//QtCreator         values.put(Data.MIMETYPE,Email.CONTENT_ITEM_TYPE);
//QtCreator         values.put(Email.DATA, email);
//QtCreator         values.put(Email.TYPE, type);
//QtCreator         QtApplication.mainActivity().getContentResolver().insert(ContactsContract.Data.CONTENT_URI, values);
//QtCreator     }
//QtCreator 
//QtCreator     private void saveNoteDetails(String note,ContentValues values,long rawContactId)
//QtCreator     {
//QtCreator         values.clear();
//QtCreator         values.put(Data.RAW_CONTACT_ID, rawContactId);
//QtCreator         values.put(Data.MIMETYPE, Note.CONTENT_ITEM_TYPE);
//QtCreator         values.put(Note.NOTE,note);
//QtCreator         QtApplication.mainActivity().getContentResolver().insert( ContactsContract.Data.CONTENT_URI, values);
//QtCreator     }
//QtCreator 
//QtCreator     private void saveNickNameDetails(String nickname,ContentValues values,long rawContactId)
//QtCreator     {
//QtCreator         values.clear();
//QtCreator         values.put(Data.RAW_CONTACT_ID, rawContactId);
//QtCreator         values.put(Data.MIMETYPE, Nickname.CONTENT_ITEM_TYPE);
//QtCreator         values.put(Nickname.NAME,nickname);
//QtCreator         QtApplication.mainActivity().getContentResolver().insert( ContactsContract.Data.CONTENT_URI, values);
//QtCreator     }
//QtCreator 
//QtCreator     private void saveBirthdayDetails(int year,int month,int date,ContentValues values,long rawContactId)
//QtCreator     {
//QtCreator         Calendar cc= Calendar.getInstance();
//QtCreator         cc.set(year, month, date);
//QtCreator         Date dt = cc.getTime();
//QtCreator         DateFormat df = DateFormat.getDateInstance(DateFormat.LONG);
//QtCreator         String birdate = df.format(dt);
//QtCreator         Log.i("BirthdayDate",birdate);
//QtCreator         values.clear();
//QtCreator         values.put(Data.RAW_CONTACT_ID, rawContactId);
//QtCreator         values.put(Data.MIMETYPE,Event.CONTENT_ITEM_TYPE);
//QtCreator         values.put(Event.START_DATE,birdate);
//QtCreator         values.put(Event.TYPE,Event.TYPE_BIRTHDAY);
//QtCreator         QtApplication.mainActivity().getContentResolver().insert(ContactsContract.Data.CONTENT_URI, values);
//QtCreator     }
//QtCreator 
//QtCreator     private void saveAnniversaryDetails(int year,int month,int date,ContentValues values,long rawContactId)
//QtCreator     {
//QtCreator         Calendar cc= Calendar.getInstance();
//QtCreator         cc.set(year, month, date);
//QtCreator         Date dt = cc.getTime();
//QtCreator         DateFormat df = DateFormat.getDateInstance(DateFormat.LONG);
//QtCreator         String annidate = df.format(dt);
//QtCreator         values.clear();
//QtCreator         values.put(Data.RAW_CONTACT_ID, rawContactId);
//QtCreator         values.put(Data.MIMETYPE,Event.CONTENT_ITEM_TYPE);
//QtCreator         values.put(Event.START_DATE,annidate);
//QtCreator         values.put(Event.TYPE,Event.TYPE_ANNIVERSARY);
//QtCreator         QtApplication.mainActivity().getContentResolver().insert(ContactsContract.Data.CONTENT_URI, values);
//QtCreator     }
//QtCreator 
//QtCreator     @SuppressWarnings("unused")
//QtCreator     private int removeContact(String id)
//QtCreator     {
//QtCreator         int deleted = 0;
//QtCreator         ContentResolver cr = QtApplication.mainActivity().getContentResolver();
//QtCreator         String where = ContactsContract.Data.CONTACT_ID + " = ?";
//QtCreator         String[] whereParams = new String[]{id};
//QtCreator         Cursor rmCur = QtApplication.mainActivity().getContentResolver().query(ContactsContract.Data.CONTENT_URI, null, where, whereParams, null);
//QtCreator         if(rmCur.moveToFirst())
//QtCreator         {
//QtCreator             try{
//QtCreator                 String lookupKey = rmCur.getString(rmCur.getColumnIndex(ContactsContract.Contacts.LOOKUP_KEY));
//QtCreator                 Uri uri = Uri.withAppendedPath(ContactsContract.Contacts.CONTENT_LOOKUP_URI, lookupKey);
//QtCreator                 cr.delete(uri,ContactsContract.Contacts._ID ,new String[] {id});
//QtCreator                 deleted = 1;
//QtCreator             }
//QtCreator             catch(Exception e)
//QtCreator             {
//QtCreator                 Log.i("QtAndroidContacts",e.toString());
//QtCreator             }
//QtCreator 
//QtCreator         }
//QtCreator         return deleted;
//QtCreator     }
//QtCreator 
//QtCreator     @SuppressWarnings("unused")
//QtCreator     private void updateContact(String id,MyContacts qtContacts)
//QtCreator     {
//QtCreator 
//QtCreator         updateNameDetails(qtContacts.m_names, id);
//QtCreator         updateNumberDetails(qtContacts.m_phoneNumbers,id);
//QtCreator         updateAddressDetails(qtContacts.m_address,id);
//QtCreator         updateOrganizationalDetails(qtContacts.m_organizations,id);
//QtCreator         updateUrlDetails(qtContacts.m_contactUrls,id);
//QtCreator         updateEmailDetails(qtContacts.m_email,id);
//QtCreator         if(qtContacts.m_contactNote.length()!=0)
//QtCreator         {
//QtCreator             updateNoteDetails(qtContacts.m_contactNote,id);
//QtCreator         }
//QtCreator         if(qtContacts.m_contactNickName.length()!=0)
//QtCreator         {
//QtCreator                     updateNicknameDetails(qtContacts.m_contactNickName,id);
//QtCreator         }
//QtCreator         if(qtContacts.m_contactBirthDay.length()!=0)
//QtCreator         {
//QtCreator             String[] birthdayInfo = qtContacts.m_contactBirthDay.split(":");
//QtCreator             int year = Integer.valueOf(birthdayInfo[0]);
//QtCreator             int month = Integer.parseInt(birthdayInfo[1], 10);
//QtCreator             month--;
//QtCreator             int day = Integer.parseInt(birthdayInfo[2],10);
//QtCreator             updateBirthdayDetails(year, month, day,id);
//QtCreator         }
//QtCreator         if(qtContacts.m_contactAnniversary.length()!=0)
//QtCreator         {
//QtCreator             String[] anniversaryInfo = qtContacts.m_contactAnniversary.split(":");
//QtCreator             int year = Integer.valueOf(anniversaryInfo[0]);
//QtCreator             int month = Integer.valueOf(anniversaryInfo[1]);
//QtCreator             month--;
//QtCreator             int day = Integer.parseInt(anniversaryInfo[2],10);
//QtCreator             updateAnniversaryDetails(year, month, day,id);
//QtCreator         }
//QtCreator     }
//QtCreator 
//QtCreator     private void updateNameDetails(NameData names,String id)
//QtCreator     {
//QtCreator         ContentValues values = new ContentValues();
//QtCreator         String nameWhere = ContactsContract.Data.CONTACT_ID + " = ? AND " + ContactsContract.Data.MIMETYPE + " = ?";
//QtCreator         String[] nameWhereParams = new String[]{id,
//QtCreator                         ContactsContract.CommonDataKinds.StructuredName.CONTENT_ITEM_TYPE};
//QtCreator         Cursor nameCur = QtApplication.mainActivity().getContentResolver().query(ContactsContract.Data.CONTENT_URI,
//QtCreator                         null, nameWhere, nameWhereParams, null);
//QtCreator         if(nameCur.moveToFirst())
//QtCreator         {
//QtCreator             String nid = nameCur.getString(nameCur.getColumnIndex(ContactsContract.CommonDataKinds.StructuredName._ID));
//QtCreator             QtApplication.mainActivity().getContentResolver().delete(ContentUris.withAppendedId(ContactsContract.Data.CONTENT_URI,Long.parseLong(nid)),null,null);
//QtCreator         }
//QtCreator         long lid = Long.parseLong(id);
//QtCreator         saveNameDetails(names, values, lid);
//QtCreator     }
//QtCreator 
//QtCreator     private void updateNumberDetails(PhoneNumber[] phoneinfo,String id)
//QtCreator     {
//QtCreator         ContentResolver cr = QtApplication.mainActivity().getContentResolver();
//QtCreator         ContentValues values = new ContentValues();
//QtCreator         Cursor pCur = QtApplication.mainActivity().getContentResolver().query(
//QtCreator                         ContactsContract.CommonDataKinds.Phone.CONTENT_URI,
//QtCreator                         null,
//QtCreator                         ContactsContract.CommonDataKinds.Phone.CONTACT_ID +" = ?",
//QtCreator                         new String[]{id}, null);
//QtCreator         while(pCur.moveToNext())
//QtCreator         {
//QtCreator             try{
//QtCreator                 String nid= pCur.getString(pCur.getColumnIndex(ContactsContract.CommonDataKinds.Phone._ID));
//QtCreator                 cr.delete(ContentUris.withAppendedId(ContactsContract.CommonDataKinds.Phone.CONTENT_URI,Long.parseLong(nid)),null,null);
//QtCreator             }
//QtCreator             catch(Exception e)
//QtCreator             {
//QtCreator                 Log.i("QtAndroidContacts",e.toString());
//QtCreator             }
//QtCreator         }
//QtCreator         pCur.close();
//QtCreator         int numberCount;
//QtCreator         numberCount = phoneinfo.length;
//QtCreator         for(int i=0;i<numberCount;i++)
//QtCreator         {
//QtCreator             if(phoneinfo[i].m_number.length()!= 0)
//QtCreator             {
//QtCreator                 savePhoneNumberDetails(phoneinfo[i].m_number,phoneinfo[i].m_type, values, Long.parseLong(id));
//QtCreator             }
//QtCreator         }
//QtCreator     }
//QtCreator 
//QtCreator     private void updateAddressDetails(AddressData[] addrinfo,String id)
//QtCreator     {
//QtCreator         ContentResolver cr = QtApplication.mainActivity().getContentResolver();
//QtCreator         String addrWhere = ContactsContract.Data.CONTACT_ID + " = ? AND " + ContactsContract.Data.MIMETYPE + " = ?";
//QtCreator         String[] addrWhereParams = new String[]{id,
//QtCreator                         ContactsContract.CommonDataKinds.StructuredPostal.CONTENT_ITEM_TYPE};
//QtCreator         Cursor addrCur = QtApplication.mainActivity().getContentResolver().query(ContactsContract.Data.CONTENT_URI,
//QtCreator                         null, addrWhere,addrWhereParams, null);
//QtCreator         ContentValues values = new ContentValues();
//QtCreator         while(addrCur.moveToNext())
//QtCreator         {
//QtCreator             String aid= addrCur.getString(addrCur.getColumnIndex(ContactsContract.CommonDataKinds.StructuredPostal._ID));
//QtCreator             cr.delete(ContentUris.withAppendedId(ContactsContract.CommonDataKinds.StructuredPostal.CONTENT_URI,Long.parseLong(aid)),null,null);
//QtCreator         }
//QtCreator         addrCur.close();
//QtCreator         int numberCount;
//QtCreator         numberCount = addrinfo.length;
//QtCreator         for(int i=0;i<numberCount;i++)
//QtCreator         {
//QtCreator             if(addrinfo[i].m_pobox.length()!= 0
//QtCreator                             || addrinfo[i].m_street.length()!= 0
//QtCreator                             || addrinfo[i].m_city.length()!= 0
//QtCreator                             || addrinfo[i].m_region.length()!= 0
//QtCreator                             || addrinfo[i].m_postCode.length()!= 0
//QtCreator                             || addrinfo[i].m_country.length()!= 0)
//QtCreator             {
//QtCreator                 saveAddressDetails(addrinfo[i].m_pobox,addrinfo[i].m_street,
//QtCreator                                 addrinfo[i].m_city,addrinfo[i].m_region,addrinfo[i].m_postCode,
//QtCreator                                 addrinfo[i].m_country,addrinfo[i].m_type,values,Long.parseLong(id));
//QtCreator             }
//QtCreator         }
//QtCreator     }
//QtCreator 
//QtCreator     private void updateOrganizationalDetails(OrganizationalData[] orginfo,String id)
//QtCreator     {
//QtCreator         ContentResolver cr = QtApplication.mainActivity().getContentResolver();
//QtCreator         ContentValues values = new ContentValues();
//QtCreator         String orgWhere = ContactsContract.Data.CONTACT_ID + " = ? AND " + ContactsContract.Data.MIMETYPE + " = ?";
//QtCreator         String[] orgWhereParams = new String[]{id,
//QtCreator                         ContactsContract.CommonDataKinds.Organization.CONTENT_ITEM_TYPE};
//QtCreator         Cursor orgCur = QtApplication.mainActivity().getContentResolver().query(ContactsContract.Data.CONTENT_URI,
//QtCreator                         null, orgWhere, orgWhereParams, null);
//QtCreator         while(orgCur.moveToNext())
//QtCreator         {
//QtCreator             String oid= orgCur.getString(orgCur.getColumnIndex(ContactsContract.CommonDataKinds.Organization._ID));
//QtCreator             cr.delete(ContentUris.withAppendedId(ContactsContract.Data.CONTENT_URI,Long.parseLong(oid)),null,null);
//QtCreator         }
//QtCreator         orgCur.close();
//QtCreator         int numberCount;
//QtCreator         numberCount = orginfo.length;
//QtCreator         for(int i=0;i<numberCount;i++)
//QtCreator         {
//QtCreator             if(orginfo[i].m_organization.length()!= 0
//QtCreator                             || orginfo[i].m_title.length()!= 0)
//QtCreator             {
//QtCreator                 saveOrganizationalDetails(orginfo[i].m_organization,orginfo[i].m_title,
//QtCreator                                 orginfo[i].m_type, values,Long.parseLong(id));
//QtCreator             }
//QtCreator         }
//QtCreator     }
//QtCreator 
//QtCreator     private void updateUrlDetails(String[] urlStrings,String id)
//QtCreator     {
//QtCreator         ContentResolver cr = QtApplication.mainActivity().getContentResolver();
//QtCreator         ContentValues values = new ContentValues();
//QtCreator         String urlWhere = ContactsContract.Data.CONTACT_ID + " = ? AND " + ContactsContract.Data.MIMETYPE + " = ?";
//QtCreator         String[] urlWhereParams = new String[]{id, ContactsContract.CommonDataKinds.Website.CONTENT_ITEM_TYPE};
//QtCreator         Cursor urlCur = QtApplication.mainActivity().getContentResolver().query(ContactsContract.Data.CONTENT_URI, null, urlWhere, urlWhereParams, null);
//QtCreator         while(urlCur.moveToNext())
//QtCreator         {
//QtCreator             String uid =urlCur.getString(urlCur.getColumnIndex(ContactsContract.CommonDataKinds.Website._ID));
//QtCreator             cr.delete(ContentUris.withAppendedId(ContactsContract.Data.CONTENT_URI,Long.parseLong(uid)),null,null);
//QtCreator         }
//QtCreator         urlCur.close();
//QtCreator         int urlCount = urlStrings.length;
//QtCreator         for(int i=0;i<urlCount;i++)
//QtCreator         {
//QtCreator             if(urlStrings[i].length()!=0)
//QtCreator             {
//QtCreator                 saveUrlDetails(urlStrings[i], values, Long.parseLong(id));
//QtCreator             }
//QtCreator         }
//QtCreator     }
//QtCreator 
//QtCreator     private void updateEmailDetails(EmailData[] emailinfo,String id)
//QtCreator     {
//QtCreator         ContentResolver cr = QtApplication.mainActivity().getContentResolver();
//QtCreator         ContentValues values = new ContentValues();
//QtCreator         Cursor emailCur = QtApplication.mainActivity().getContentResolver().query(
//QtCreator                         ContactsContract.CommonDataKinds.Email.CONTENT_URI,
//QtCreator                         null,
//QtCreator                         ContactsContract.CommonDataKinds.Email.CONTACT_ID + " = ?",
//QtCreator                                         new String[]{id}, null);
//QtCreator         while(emailCur.moveToNext())
//QtCreator         {
//QtCreator             String eid = emailCur.getString(emailCur.getColumnIndex(ContactsContract.CommonDataKinds.Email._ID));
//QtCreator             cr.delete(ContentUris.withAppendedId(ContactsContract.CommonDataKinds.Email.CONTENT_URI,Long.parseLong(eid)),null,null);
//QtCreator         }
//QtCreator         emailCur.close();
//QtCreator         int emailCount = emailinfo.length;
//QtCreator         for(int i=0;i<emailCount;i++)
//QtCreator         {
//QtCreator             if(emailinfo[i].m_email.length()!=0)
//QtCreator             {
//QtCreator                 SaveEmailDetails(emailinfo[i].m_email,emailinfo[i].m_type, values,Long.parseLong(id));
//QtCreator             }
//QtCreator         }
//QtCreator     }
//QtCreator 
//QtCreator     private void updateNoteDetails(String note,String id)
//QtCreator     {
//QtCreator         ContentResolver cr = QtApplication.mainActivity().getContentResolver();
//QtCreator         ContentValues values = new ContentValues();
//QtCreator         values.put(Note.NOTE,note);
//QtCreator         String noteWhere = ContactsContract.Data.CONTACT_ID + " = ? AND " + ContactsContract.Data.MIMETYPE + " = ?";
//QtCreator         String[] noteWhereParams = new String[]{id, ContactsContract.CommonDataKinds.Note.CONTENT_ITEM_TYPE};
//QtCreator         Cursor noteCur = QtApplication.mainActivity().getContentResolver().query(ContactsContract.Data.CONTENT_URI, null, noteWhere, noteWhereParams, null);
//QtCreator         if (noteCur.moveToFirst()) {
//QtCreator             String nid = noteCur.getString(noteCur.getColumnIndex(ContactsContract.CommonDataKinds.Note._ID));
//QtCreator             cr.delete(ContentUris.withAppendedId(ContactsContract.Data.CONTENT_URI,Long.parseLong(nid)),null,null);
//QtCreator         }
//QtCreator         noteCur.close();
//QtCreator         saveNoteDetails(note, values,Long.parseLong(id));
//QtCreator     }
//QtCreator 
//QtCreator     private void updateNicknameDetails(String nickname,String id)
//QtCreator     {
//QtCreator         ContentResolver cr = QtApplication.mainActivity().getContentResolver();
//QtCreator         ContentValues values = new ContentValues();
//QtCreator         String nickWhere = ContactsContract.Data.CONTACT_ID + " = ? AND " + ContactsContract.Data.MIMETYPE + " = ?";
//QtCreator         String[] nickWhereParams = new String[]{id,
//QtCreator                         ContactsContract.CommonDataKinds.Nickname.CONTENT_ITEM_TYPE};
//QtCreator         Cursor nickCur = QtApplication.mainActivity().getContentResolver().query(ContactsContract.Data.CONTENT_URI,
//QtCreator                         null, nickWhere, nickWhereParams, null);
//QtCreator         if (nickCur.moveToFirst()) {
//QtCreator             String nid= nickCur.getString(nickCur.getColumnIndex(ContactsContract.CommonDataKinds.Nickname._ID));
//QtCreator             cr.delete(ContentUris.withAppendedId(ContactsContract.Data.CONTENT_URI,Long.parseLong(nid)),null,null);
//QtCreator         }
//QtCreator         nickCur.close();
//QtCreator         saveNickNameDetails(nickname, values,Long.parseLong(id));
//QtCreator     }
//QtCreator 
//QtCreator     private void updateBirthdayDetails(int year,int month,int date,String id)
//QtCreator     {
//QtCreator         ContentResolver cr = QtApplication.mainActivity().getContentResolver();
//QtCreator         String where = ContactsContract.Data.MIMETYPE + " = ? ";
//QtCreator         String[] whereParams = new String[] {ContactsContract.CommonDataKinds.Event.CONTENT_ITEM_TYPE};
//QtCreator         ContentValues values = new ContentValues();
//QtCreator         Cursor birthDayCur =QtApplication.mainActivity().getContentResolver().query( ContactsContract.Data.CONTENT_URI, new String[] { Event.DATA }, ContactsContract.Data.CONTACT_ID + "=" + id + " AND " + Data.MIMETYPE + "= '" + Event.CONTENT_ITEM_TYPE + "' AND " + Event.TYPE + "=" + Event.TYPE_BIRTHDAY, null, null);
//QtCreator         if(birthDayCur.moveToFirst())
//QtCreator         {
//QtCreator             cr.delete(ContactsContract.Data.CONTENT_URI,where,whereParams);
//QtCreator         }
//QtCreator         birthDayCur.close();
//QtCreator         saveBirthdayDetails(year, month, date, values,Long.parseLong(id));
//QtCreator     }
//QtCreator 
//QtCreator     private void updateAnniversaryDetails(int year,int month,int date,String id)
//QtCreator     {
//QtCreator         ContentResolver cr = QtApplication.mainActivity().getContentResolver();
//QtCreator         String where = ContactsContract.Data.MIMETYPE + " = ? ";
//QtCreator         String[] whereParams = new String[]{Event.CONTENT_ITEM_TYPE};
//QtCreator         ContentValues values = new ContentValues();
//QtCreator         Cursor anniversaryCur =QtApplication.mainActivity().getContentResolver().query( ContactsContract.Data.CONTENT_URI, new String[] { Event.DATA }, ContactsContract.Data.CONTACT_ID + "=" + id + " AND " + Data.MIMETYPE + "= '" + Event.CONTENT_ITEM_TYPE + "' AND " + Event.TYPE + "=" + Event.TYPE_ANNIVERSARY, null, ContactsContract.Data.DISPLAY_NAME);
//QtCreator         if(anniversaryCur.moveToFirst())
//QtCreator         {
//QtCreator             cr.delete(ContactsContract.Data.CONTENT_URI,where,whereParams);
//QtCreator         }
//QtCreator         anniversaryCur.close();
//QtCreator         saveAnniversaryDetails(year, month, date, values,Long.parseLong(id));
//QtCreator     }
//QtCreator 
//QtCreator }
//QtCreator 
//QtCreator class AndroidContacts
//QtCreator {
//QtCreator     public MyContacts[] m_allAndroidContacts;
//QtCreator 
//QtCreator     public AndroidContacts(int count)
//QtCreator     {
//QtCreator         m_allAndroidContacts = new MyContacts[count];
//QtCreator     }
//QtCreator 
//QtCreator     public void buildContacts(int i,String id,String name,NameData names,PhoneNumber[] phoneNumberArray,
//QtCreator                                     EmailData[] emailArray,String note,AddressData[] addressArray,
//QtCreator                                     OrganizationalData[] orgArray,String birthday,String anniversary,
//QtCreator                                     String nickName,String[] urlArray,OnlineAccount[] onlineAccountArray)
//QtCreator     {
//QtCreator         m_allAndroidContacts[i] = new MyContacts(id,name,names,phoneNumberArray,emailArray,note,addressArray,orgArray,birthday,anniversary,nickName,urlArray,onlineAccountArray);
//QtCreator     }
//QtCreator }
//QtCreator 
//QtCreator class MyContacts
//QtCreator {
//QtCreator     String   m_dispalyName;
//QtCreator     NameData m_names;
//QtCreator     PhoneNumber[] m_phoneNumbers;
//QtCreator     EmailData[] m_email;
//QtCreator     String   m_contactNote;
//QtCreator     AddressData[] m_address;
//QtCreator     OrganizationalData[] m_organizations;
//QtCreator     OnlineAccount[] m_onlineAccount;
//QtCreator     String   m_contactID;
//QtCreator     String   m_contactBirthDay;
//QtCreator     String   m_contactAnniversary;
//QtCreator     String   m_contactNickName;
//QtCreator     String[] m_contactUrls;
//QtCreator     public MyContacts(NameData names,PhoneNumber[] phoneNumberArray,EmailData[] emailArray,String note,AddressData[] addressArray,OrganizationalData[] organizationArray,String birthday,String anniversary,String nickName,String[] urlArray)
//QtCreator     {
//QtCreator         m_names = new NameData(names.m_firstName,names.m_lastName,names.m_middleName,names.m_prefix,names.m_suffix);
//QtCreator         int i=0;
//QtCreator         int numbercount = phoneNumberArray.length;
//QtCreator         if(numbercount>0)
//QtCreator         {
//QtCreator             m_phoneNumbers = new PhoneNumber[numbercount];
//QtCreator             for(i=0;i<numbercount;i++)
//QtCreator             {
//QtCreator                 m_phoneNumbers[i] = phoneNumberArray[i];
//QtCreator             }
//QtCreator         }
//QtCreator 
//QtCreator         int emailcount = emailArray.length;
//QtCreator         if(emailcount>0)
//QtCreator         {
//QtCreator             m_email = new EmailData[emailcount];
//QtCreator             for(i=0;i<emailcount;i++)
//QtCreator             {
//QtCreator                 m_email[i] = emailArray[i];
//QtCreator             }
//QtCreator         }
//QtCreator         if(note != null)
//QtCreator         {
//QtCreator             m_contactNote = note;
//QtCreator         }
//QtCreator 
//QtCreator         int addrcount = addressArray.length;
//QtCreator         m_address = new AddressData[addrcount];
//QtCreator         for(i=0;i<addrcount;i++)
//QtCreator         {
//QtCreator             m_address[i] = addressArray[i];
//QtCreator         }
//QtCreator 
//QtCreator         int orgcount = organizationArray.length;
//QtCreator         m_organizations = new OrganizationalData[orgcount];
//QtCreator         for(i=0;i<orgcount;i++)
//QtCreator         {
//QtCreator             m_organizations[i] = organizationArray[i];
//QtCreator         }
//QtCreator 
//QtCreator         if(birthday != null)
//QtCreator         {
//QtCreator             m_contactBirthDay = birthday;
//QtCreator         }
//QtCreator 
//QtCreator         if(anniversary != null)
//QtCreator         {
//QtCreator             m_contactAnniversary = anniversary;
//QtCreator         }
//QtCreator 
//QtCreator         if(nickName != null)
//QtCreator         {
//QtCreator             m_contactNickName = nickName;
//QtCreator         }
//QtCreator 
//QtCreator         int urlcount = urlArray.length;
//QtCreator         if(urlcount>0)
//QtCreator         {
//QtCreator             m_contactUrls = new String[urlcount];
//QtCreator             for(i=0;i<urlcount;i++)
//QtCreator             {
//QtCreator                 m_contactUrls[i] = urlArray[i];
//QtCreator             }
//QtCreator         }
//QtCreator     }
//QtCreator 
//QtCreator     public MyContacts(String iid,String name,NameData names,PhoneNumber[] phonenumberArray,
//QtCreator                     EmailData[] emailArray,String note,AddressData[] addressArray,
//QtCreator                     OrganizationalData[] organizationArray,String birthday,
//QtCreator                     String anniversary,String nickName,String[] urlArray,OnlineAccount[] onlineAccountArray)
//QtCreator     {
//QtCreator         m_contactID = iid;
//QtCreator         m_dispalyName = name;
//QtCreator         if(names != null)
//QtCreator         {
//QtCreator             m_names = new NameData(names.m_firstName,names.m_lastName,names.m_middleName,names.m_prefix,names.m_suffix);
//QtCreator         }
//QtCreator         else
//QtCreator         {
//QtCreator             m_names = new NameData("","","","","");
//QtCreator         }
//QtCreator         int i=0;
//QtCreator 
//QtCreator         if(phonenumberArray.length>0)
//QtCreator         {
//QtCreator             m_phoneNumbers = new PhoneNumber[phonenumberArray.length];
//QtCreator             for(i=0;i<phonenumberArray.length;i++)
//QtCreator             {
//QtCreator                 m_phoneNumbers[i] = phonenumberArray[i];
//QtCreator             }
//QtCreator         }
//QtCreator         else
//QtCreator         {
//QtCreator             m_phoneNumbers = new PhoneNumber[1];
//QtCreator             m_phoneNumbers[0] = new PhoneNumber("", 0);
//QtCreator         }
//QtCreator 
//QtCreator         if(emailArray.length>0)
//QtCreator         {
//QtCreator             m_email = new EmailData[emailArray.length];
//QtCreator             for(i=0;i<emailArray.length;i++)
//QtCreator             {
//QtCreator                 m_email[i] = emailArray[i];
//QtCreator             }
//QtCreator         }
//QtCreator         else
//QtCreator         {
//QtCreator             m_email = new EmailData[1];
//QtCreator             m_email[0] = new EmailData("",0);
//QtCreator         }
//QtCreator 
//QtCreator         if(note != null)
//QtCreator         {
//QtCreator             m_contactNote = note;
//QtCreator         }
//QtCreator         else
//QtCreator         {
//QtCreator             m_contactNote = "";
//QtCreator         }
//QtCreator 
//QtCreator         if(addressArray.length>0)
//QtCreator         {
//QtCreator             m_address = new AddressData[addressArray.length];
//QtCreator             for(i=0;i<addressArray.length;i++)
//QtCreator             {
//QtCreator                 m_address[i] = addressArray[i];
//QtCreator             }
//QtCreator         }
//QtCreator         else
//QtCreator         {
//QtCreator             m_address = new AddressData[1];
//QtCreator             m_address[0] = new AddressData("","", "", "", "", "", 0);
//QtCreator         }
//QtCreator 
//QtCreator         if(organizationArray.length>0)
//QtCreator         {
//QtCreator             m_organizations = new OrganizationalData[organizationArray.length];
//QtCreator             for(i=0;i<organizationArray.length;i++)
//QtCreator             {
//QtCreator                 m_organizations[i] = organizationArray[i];
//QtCreator             }
//QtCreator         }
//QtCreator         else
//QtCreator         {
//QtCreator             m_organizations = new OrganizationalData[1];
//QtCreator             m_organizations[0] = new OrganizationalData("","",0);
//QtCreator         }
//QtCreator 
//QtCreator 
//QtCreator         if(birthday != null)
//QtCreator         {
//QtCreator             m_contactBirthDay = birthday;
//QtCreator         }
//QtCreator         else
//QtCreator         {
//QtCreator             m_contactBirthDay ="";
//QtCreator         }
//QtCreator 
//QtCreator         if(anniversary != null)
//QtCreator         {
//QtCreator             m_contactAnniversary = anniversary;
//QtCreator         }
//QtCreator         else
//QtCreator         {
//QtCreator             m_contactAnniversary ="";
//QtCreator         }
//QtCreator 
//QtCreator         if(nickName != null)
//QtCreator         {
//QtCreator             m_contactNickName = nickName;
//QtCreator         }
//QtCreator         else
//QtCreator         {
//QtCreator             m_contactNickName = "";
//QtCreator         }
//QtCreator 
//QtCreator         if(urlArray.length>0)
//QtCreator         {
//QtCreator             m_contactUrls = new String[urlArray.length];
//QtCreator             for(i=0;i<urlArray.length;i++)
//QtCreator             {
//QtCreator                 m_contactUrls[i] = urlArray[i];
//QtCreator             }
//QtCreator         }
//QtCreator         else
//QtCreator         {
//QtCreator             m_contactUrls = new String[1];
//QtCreator             m_contactUrls[0]= "";
//QtCreator         }
//QtCreator 
//QtCreator 
//QtCreator         if(onlineAccountArray.length>0)
//QtCreator         {
//QtCreator             m_onlineAccount = new OnlineAccount[onlineAccountArray.length];
//QtCreator             for(i=0;i<onlineAccountArray.length;i++)
//QtCreator             {
//QtCreator                 m_onlineAccount[i] = onlineAccountArray[i];
//QtCreator             }
//QtCreator         }
//QtCreator         else
//QtCreator         {
//QtCreator             m_onlineAccount = new OnlineAccount[1];
//QtCreator             m_onlineAccount[0] = new OnlineAccount("","",0,0, 0, 0);
//QtCreator         }
//QtCreator     }
//QtCreator }
//QtCreator 
//QtCreator class PhoneNumber
//QtCreator {
//QtCreator     String m_number;
//QtCreator     int m_type;
//QtCreator     public PhoneNumber(String number,int type)
//QtCreator     {
//QtCreator         m_number = number;
//QtCreator         m_type = type;
//QtCreator     }
//QtCreator }
//QtCreator 
//QtCreator class EmailData
//QtCreator {
//QtCreator     String m_email;
//QtCreator     int m_type;
//QtCreator     public EmailData(String email,int type)
//QtCreator     {
//QtCreator         m_email = email;
//QtCreator         m_type = type;
//QtCreator     }
//QtCreator 
//QtCreator }
//QtCreator 
//QtCreator class OrganizationalData
//QtCreator {
//QtCreator     String m_organization;
//QtCreator     String m_title;
//QtCreator     int m_type;
//QtCreator     public OrganizationalData(String organization,String title,int type)
//QtCreator     {
//QtCreator         m_organization = organization;
//QtCreator         m_title = title;
//QtCreator         m_type = type;
//QtCreator     }
//QtCreator }
//QtCreator 
//QtCreator class AddressData
//QtCreator {
//QtCreator     String m_pobox;
//QtCreator     String m_street;
//QtCreator     String m_city;
//QtCreator     String m_region;
//QtCreator     String m_postCode;
//QtCreator     String m_country;
//QtCreator     int m_type;
//QtCreator     public AddressData(String pobox,String street,String city,String region,String postCode,String country,int type)
//QtCreator     {
//QtCreator         m_pobox = pobox;
//QtCreator         m_street = street;
//QtCreator         m_city = city;
//QtCreator         m_region = region;
//QtCreator         m_postCode = postCode;
//QtCreator         m_country = country;
//QtCreator         m_type = type;
//QtCreator     }
//QtCreator }
//QtCreator 
//QtCreator class NameData
//QtCreator {
//QtCreator     String m_firstName;
//QtCreator     String m_lastName;
//QtCreator     String m_middleName;
//QtCreator     String m_prefix;
//QtCreator     String m_suffix;
//QtCreator     public NameData(String firstName,String lastName,String middleName,String prefix,String suffix)
//QtCreator     {
//QtCreator         m_firstName = firstName;
//QtCreator         m_lastName = lastName;
//QtCreator         m_middleName = middleName;
//QtCreator         m_prefix = prefix;
//QtCreator         m_suffix = suffix;
//QtCreator     }
//QtCreator }
//QtCreator 
//QtCreator class OnlineAccount
//QtCreator {
//QtCreator     String m_account;
//QtCreator     String m_status;
//QtCreator     String m_customProtocol;
//QtCreator     long m_timeStamp;
//QtCreator     int m_presence;
//QtCreator     int m_protocol;
//QtCreator     int m_type;
//QtCreator 
//QtCreator 
//QtCreator     public OnlineAccount(String account,String status,long timeStamp,int presence,int protocol,int type)
//QtCreator     {
//QtCreator         m_account = account;
//QtCreator         m_status = status;
//QtCreator         m_timeStamp = timeStamp;
//QtCreator         m_presence = presence;
//QtCreator         m_protocol = protocol;
//QtCreator         m_type = type;
//QtCreator     }
//QtCreator 
//QtCreator     public OnlineAccount(String account,String status,long timeStamp,String customProtocol,int presence,int protocol,int type)
//QtCreator     {
//QtCreator         m_account = account;
//QtCreator         m_status = status;
//QtCreator         m_customProtocol = customProtocol;
//QtCreator         m_timeStamp = timeStamp;
//QtCreator         m_presence = presence;
//QtCreator         m_protocol = protocol;
//QtCreator         m_type = type;
//QtCreator     }
//QtCreator }
//@ANDROID-5
