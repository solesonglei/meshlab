/****************************************************************************
 * MeshLab                                                           o o     *
 * A versatile mesh processing toolbox                             o     o   *
 *                                                                _   O  _   *
 * Copyright(C) 2005                                                \/)\/    *
 * Visual Computing Lab                                            /\/|      *
 * ISTI - Italian National Research Council                           |      *
 *                                                                    \      *
 * All rights reserved.                                                      *
 *                                                                           *
 * This program is free software; you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation; either version 2 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
 * for more details.                                                         *
 *                                                                           *
 ****************************************************************************/

#include "paintbox.h"

Paintbox::Paintbox(QWidget * parent, Qt::WindowFlags flags) : QWidget(parent, flags)
{
	setupUi(this);
	
	setUndoStack(new QUndoStack(this));	
	
	active = COLOR_PAINT;
	gradient_frame->setHidden(true);
	pick_frame->setHidden(true);
	smooth_frame->setHidden(true);
	mesh_displacement_frame->setHidden(true);
	clone_source_frame->setHidden(true);
	noise_frame->setHidden(true);
	
	brush_viewer->setScene(new QGraphicsScene());
	clone_source_view->setScene(new QGraphicsScene());
	clone_source_view->centerOn(0, 0);
	
	item = NULL;
	pixmap_available = false;
	
	//******QT 4.3 Workaround**********
	QScrollArea * scrollArea = new QScrollArea(this);
	gridLayout->removeWidget(widget);
	scrollArea->setWidget(widget);
	static_cast<QGridLayout * >(widget->layout())->addItem(new QSpacerItem(0, 20, QSizePolicy::Minimum, QSizePolicy::Expanding), 11, 0, 1, 2);
	widget->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding));
	scrollArea->setFrameStyle(QFrame::NoFrame);
	scrollArea->setWidgetResizable(true);
	scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scrollArea->adjustSize();
	gridLayout->addWidget(scrollArea, 2, 1, 1, 1);
	//**********************************
	
	refreshBrushPreview();
}

void Paintbox::setUndoStack(QUndoStack * qus)
{
	stack = qus;
	
	QIcon undo = undo_button->icon();
	undo_button->setDefaultAction(stack->createUndoAction(undo_button));
	undo_button->defaultAction()->setIcon(undo);

	QIcon redo = redo_button->icon();
	redo_button->setDefaultAction(stack->createRedoAction(redo_button));
	redo_button->defaultAction()->setIcon(redo);
}

void Paintbox::on_default_colors_clicked()
{
	foreground_frame->setColor(Qt::black);
	background_frame->setColor(Qt::white);
}

void Paintbox::on_switch_colors_clicked()
{
	QColor temp = foreground_frame->getColor();
	foreground_frame->setColor(background_frame->getColor()); 
	background_frame->setColor(temp);
}

void Paintbox::setClonePixmap(QImage & image)
{
	if (item != NULL) getCloneScene()->removeItem(item);
	item = getCloneScene()->addPixmap(QPixmap::fromImage(image));
	item->setPos(0, 0);
	clone_source_view->centerOn(0, 0);
	QPen pen;
	getCloneScene()->addLine(0, 8, 0, -8, pen);
	getCloneScene()->addLine(8, 0, -8, 0, pen);	
}
	
void Paintbox::setPixmapCenter(qreal x, qreal y)
{
	item->setPos(x /*+ clone_source_view->width()/2.0*/, y /*+ clone_source_view->height()/2.0*/);
	clone_source_view->centerOn(0, 0);
}

void Paintbox::loadClonePixmap()
{
	QString s = QFileDialog::getOpenFileName(this,
		tr("Open Image"), "", tr("Image Files (*.png *.jpg *.bmp)"));
	if (!s.isNull()) 
	{
		QPixmap pixmap(s);
		if (item != NULL) getCloneScene()->removeItem(item);
		item = getCloneScene()->addPixmap(pixmap);
		item->setPos(-pixmap.width()/2.0, -pixmap.height()/2.0);
		getCloneScene()->setSceneRect(-pixmap.width()/2.0, -pixmap.height()/2.0, pixmap.width(), pixmap.height());
		clone_source_view->centerOn(0, 0);
		pixmap_available = true;
		QPen pen;
		getCloneScene()->addLine(0, 8, 0, -8, pen);
		getCloneScene()->addLine(8, 0, -8, 0, pen);	
	}	
}

void Paintbox::getPixmapBuffer(GLubyte * & buffer, GLfloat* & zbuffer, int & w, int & h)
{
	QImage image = item->pixmap().toImage();
	
	buffer = new GLubyte[image.size().height() * image.size().width() * 3];
	zbuffer = new GLfloat[image.size().height() * image.size().width()];

	for (int x = 0; x < image.size().width(); x++){
			for (int y = 0; y < image.size().height(); y++)
			{
				int index = y * image.size().width() + x;
				zbuffer[index] = 0.0;
				index *= 3;
				buffer[index] = qRed(image.pixel(x, image.size().height() - 1 - y ));
				buffer[index + 1] = qGreen(image.pixel(x, image.size().height()- 1 - y));
				buffer[index + 2] = qBlue(image.pixel(x, image.size().height()- 1 - y));
			}
		}
	w = image.size().width();
	h = image.size().height();
	pixmap_available = false;
}

void Paintbox::restorePreviousType()
{
	//TODO Only works as long as types are declared in the same order as buttons!
//	dynamic_cast<QToolButton *>(verticalLayout->itemAt(previous_type)->widget())->toggle() ;
}

void Paintbox::refreshBrushPreview()
{
	if (item != NULL) brush_viewer->scene()->removeItem(item);
		
		item = brush_viewer->scene()->addPixmap(QPixmap::fromImage(
				raster(getBrush(), (int) ((brush_viewer->width()-2) * size_slider->value() / 100.0), 
						(int)((brush_viewer->height()-2) * size_slider->value() / 100.0), getHardness())
				)
		);
		
		brush_viewer->setSceneRect(item->boundingRect());
}

void Paintbox::setForegroundColor(QColor & c)
{
	foreground_frame->setColor(c);
}

void Paintbox::setBackgroundColor(QColor & c)
{
	background_frame->setColor(c);
}
