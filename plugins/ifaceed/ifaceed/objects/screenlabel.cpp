#include "screenlabel.h"
#include "screentemplate.h"
#include "../core/fonttemplatesdatabase.h"
#include "../core/fontdatabase.h"
#include "../core/ifaceeditor.h"
#include "extra/rigid_body.h"

ScreenLabel::ScreenLabel() : AbstractScreenObject()
{
	this->invalidateCache();

	this->addProperty("font" ,new MappedField<hst::string>(&m_font_name, ""));
	this->addProperty("size" ,new MappedField<unsigned int>(&m_font_size, 0));
	this->addProperty("color",new MappedField<hst::color>(&m_font_color, hst::color(0,0,0)));
	this->addProperty("text" ,new MappedField<hst::string>(&m_text, ""));
	this->addProperty("angle",new MappedField<float>(&m_angle, 0.0f));
	this->addProperty("pos"  ,new MappedField<hPointF>(&m_point, hPointF(0,0)));
}


void ScreenLabel::invalidateCache()
{
	m_font = NULL;
}

hst::string ScreenLabel::type()
{
	return "ScreenLabel";
}

void ScreenLabel::_render()
{
	if (this->screenTemplate() == NULL)
		return;
	if (!m_font && !isValid(this->screenTemplate()))
	{
		return;
	}
	if (!m_font)
	{
		IFaceEditorFontList & d = static_cast<IFaceEditor*>(this->screenTemplate()->editor())->database()->fonts();
		IFaceEditorFont * fnt = d.font(m_font_name.data());
		if (!fnt)
			return;
		m_font = fnt->sadFont();
	}
	m_font->setColor(hst::acolor(m_font_color.r(),m_font_color.g(),m_font_color.b(),0));
	m_font->setHeight(m_font_size);

	hRectF s = m_font->size(m_text);
	glMatrixMode(GL_MODELVIEW_MATRIX);
	glPushMatrix();
	glTranslatef(m_point.x() +  s.width() /2, m_point.y() - s.height() /2, 0.0f);
	glRotatef(m_angle / M_PI*180.0f, 0.0f, 0.0f, 1.0f);
	m_font->render(m_text, - (s.width()/2), (s.height()/2));	
	
	glPopMatrix();

	hRectF r = this->region();

	// TODO: Move it into selection of object
	glDisable(GL_TEXTURE_2D);
	GLint   clr[4]={};
	glGetIntegerv(GL_CURRENT_COLOR,clr);
	glColor4ub(255,0,0,255);
    glBegin(GL_LINES);
	glVertex2f(r[0].x(), r[0].y());
	glVertex2f(r[1].x(), r[1].y());

	glVertex2f(r[1].x(), r[1].y());
	glVertex2f(r[2].x(), r[2].y());

	glVertex2f(r[2].x(), r[2].y());
	glVertex2f(r[3].x(), r[3].y());	

	glVertex2f(r[3].x(), r[3].y());	
	glVertex2f(r[0].x(), r[0].y());

	glEnd();
	glColor4iv(clr);
	glEnable(GL_TEXTURE_2D);
	

	m_angle += 0.05;
}

bool ScreenLabel::isValid(ScreenTemplate * t)
{
	IFaceEditorFontList & d = static_cast<IFaceEditor*>(t->editor())->database()->fonts();
	return d.hasFont(m_font_name.data());
}

static bool testcollision(const ::s3d::point & test,const ::s3d::point & pivot1, const ::s3d::point & pivot2)
{
	vector32 axle = pivot2-pivot1;
	normalize(axle);
	axle = ortho(axle);
	float  test_projection = scalar(test,axle);
	float  pivot1_projection = scalar(pivot1,axle);
	float  pivot2_projection = scalar(pivot2,axle);
	return collides1D(test_projection, test_projection , pivot1_projection, pivot2_projection);
}

bool ScreenLabel::isWithin(const hPointF & p)
{
	hRectF r = this->region();
	::s3d::point test =::s3d::point(p.x(),p.y(),0.0f);
	::s3d::point rp[3];
	for (int i=0;i<3;i++)
	{
		rp[3]=  ::s3d::point(r[i].x(),r[i].y(),0.0f);
	}
	
	return testcollision(test,rp[0],rp[1]) && testcollision(test,rp[1],rp[2]); 
}

hRectF ScreenLabel::region()
{
	if (this->screenTemplate() == NULL)
		return hRectF();
	if (!m_font && !isValid(this->screenTemplate()))
	{
		return hRectF();
	}
	if (!m_font)
	{
		IFaceEditorFontList & d = static_cast<IFaceEditor*>(this->screenTemplate()->editor())->database()->fonts();
		IFaceEditorFont * fnt = d.font(m_font_name.data());
		if (!fnt)
			return hRectF();
		m_font = fnt->sadFont();
	}
	m_font->setHeight(m_font_size);

	hRectF s = m_font->size(m_text);
	hRectF non_rotated_region( hPointF(s.width() / -2.0f, s.height() / -2.0f),
		                       hPointF(s.width() / 2.0f, s.height() / 2.0f));
	hPointF results[4];
	float cos_angle = cos(m_angle);
	float sin_angle = sin(m_angle);
	float centerx = m_point.x() + s.width() / 2;
	float centery = m_point.y() - s.height() / 2;
	for (int i=0;i<4;i++)
	{
		results[i].setX(centerx + non_rotated_region[i].x()*cos_angle - non_rotated_region[i].y() * sin_angle);
		results[i].setY(centery + non_rotated_region[i].x()*sin_angle + non_rotated_region[i].y() * cos_angle);
	}
	return hRectF(results[0],results[1], results[2], results[3]);
}
