#ifndef TEXTUREPACKER_H
#define TEXTUREPACKER_H

// The default max size is 2048 just to be on the safe side of portability
#define MAX_TEXTURE_SIZE 2048
// The padding is to avoid edge bleeding on GPUs that don't support texture edge clamping
#define TEXTURE_PADDING 1
// The max number of images to generate if debugging
#define MAX_IMAGE_COUNT 1024

#include <vector>
#include <stack>
#include <random>
#include <SFML/Graphics.hpp>

#include "Log.h"


struct SubImage
{
	sf::Image m_image;
	std::string	m_name;
};

struct TextureNode
{
	sf::IntRect	m_rect;
	std::string	m_name;
	size_t m_big_node;
	size_t m_small_node;
};

struct TexturePack
{
	TexturePack(unsigned int width, unsigned int height)
	{
		if(!m_texture.create(width, height))
		{
			moony::logError() << "Could not create Texture Atlas\n";
			abort();
		}

		m_image_count = 0;
		m_crop_size = sf::Vector2u(0, 0);
		TextureNode root;
		root.m_rect = sf::IntRect(0, 0, width, height);
		root.m_small_node = 0;
		root.m_big_node = 0;

		m_node_list.push_back(root);
	}

	bool placeSubImage(SubImage subimage, sf::Vector2u size, size_t index = 0)
	{
		// Check whether the current node has children
		if(!m_node_list[index].m_small_node && !m_node_list[index].m_big_node)
		{
			// Make sure that the image fits inside the indexed node
			if(size.x <= m_node_list[index].m_rect.width && size.y <= m_node_list[index].m_rect.height)
			{
				TextureNode small_node = m_node_list[index];
				TextureNode large_node = m_node_list[index];

				// Split the orignal node into two smaller rects
				large_node.m_rect.top += size.y;
				large_node.m_rect.height -= size.y;

				small_node.m_rect.left += size.x;
				small_node.m_rect.width -= size.x;
				small_node.m_rect.height = size.y;

				m_image_count++;

				if(m_node_list[index].m_rect.left + size.x > m_crop_size.x)
					m_crop_size.x = m_node_list[index].m_rect.left + size.x;

				if(m_node_list[index].m_rect.top + size.y > m_crop_size.y)
					m_crop_size.y = m_node_list[index].m_rect.top + size.y;

				// Push the children nodes onto the list and store their index in the parent node
				m_node_list.push_back(small_node);
				m_node_list[index].m_small_node = m_node_list.size() - 1;

				m_node_list.push_back(large_node);
				m_node_list[index].m_big_node = m_node_list.size() - 1;

				// Set the indexed node with the image name and size
				m_node_list[index].m_name = subimage.m_name;
				m_node_list[index].m_rect.width = size.x - TEXTURE_PADDING;
				m_node_list[index].m_rect.height = size.y - TEXTURE_PADDING;

				// Update the texture with the image data and the position of the node
				m_texture.update(subimage.m_image, m_node_list[index].m_rect.left, m_node_list[index].m_rect.top);

				moony::logDebug() << "Placing " << subimage.m_name
					<< " at " << m_node_list[index].m_rect.left << ", " << m_node_list[index].m_rect.top << "\n";

				return true;
			}

			return false;
		}
		else
		{
			if(placeSubImage(subimage, size, m_node_list[index].m_small_node))
				return true;
			else
				return placeSubImage(subimage, size, m_node_list[index].m_big_node);
		}
	}

	sf::Image getCropImage()
	{
		sf::Texture temp;
		temp.loadFromImage(m_texture.copyToImage(), sf::IntRect(0, 0, m_crop_size.x, m_crop_size.y));
		return temp.copyToImage();
	}

	size_t m_image_count;
	sf::Vector2u m_crop_size;
	sf::Texture m_texture;
	std::vector<TextureNode> m_node_list;
};

// Generates a list of debug images of varying sizes
std::vector<SubImage> generateDebugImages(unsigned int count)
{
	if(count > MAX_IMAGE_COUNT)
		count = MAX_IMAGE_COUNT;

	std::vector<SubImage> subimage_list;

	for(unsigned int index = 0; index < count; index++)
	{
		SubImage subimage;

		// "Randomly" generate a color 0 to 255 and a size
		sf::Color color(std::rand() % 255, std::rand() % 255, std::rand() % 255);
		sf::Vector2u size(32 + (std::rand() % (MAX_TEXTURE_SIZE / 16)), 32 + (std::rand() % (MAX_TEXTURE_SIZE / 16)));

		// Create the image, name it and push it to the list
		subimage.m_image.create(size.x, size.y, color);
		subimage.m_name = "debug_image" + std::to_string(index);

		subimage_list.push_back(subimage);
	}

	return subimage_list;
}

#endif